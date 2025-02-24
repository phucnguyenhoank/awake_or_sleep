from flask import Flask, request, jsonify, render_template_string, redirect, url_for
import pandas as pd
import datetime

app = Flask(__name__)

# Global variables
collecting_data = False
data = []
csv_file = None
current_label = "awake"

# Sample threshold for 20 minutes at 50 Hz
SAMPLE_THRESHOLD = 60000  # 50 Hz * 60 s * 20 min

# Enhanced HTML Template
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-C3 Data Collection</title>
    <link rel="stylesheet" href="{{ url_for('static', filename='style.css') }}">
</head>
<body>
    <div class="container">
        <h1>ESP32-C3 Data Collection</h1>
        <div class="status-box">
            <p>Status: <span class="status {{ 'active' if collecting_data else 'inactive' }}">{{ status }}</span></p>
            <p>Current Label: <span class="label">{{ label }}</span></p>
        </div>
        
        <!-- Label Selection -->
        <form action="/set_label" method="post" class="form-group">
            <label for="label">Select Label:</label>
            <select name="label" id="label" onchange="this.form.submit()">
                <option value="awake" {% if label == 'awake' %}selected{% endif %}>Awake</option>
                <option value="sleep" {% if label == 'sleep' %}selected{% endif %}>Sleep</option>
            </select>
        </form>
        
        <!-- Control Buttons -->
        <div class="button-group">
            <form action="/start" method="post" class="button-form">
                <button type="submit" class="btn btn-start" {% if collecting_data %}disabled{% endif %}>Start</button>
            </form>
            <form action="/stop" method="post" class="button-form">
                <button type="submit" class="btn btn-stop" {% if not collecting_data %}disabled{% endif %}>Stop</button>
            </form>
        </div>
    </div>
</body>
</html>
"""

@app.route("/")
def index():
    status = "Collecting Data" if collecting_data else "Idle"
    return render_template_string(HTML_TEMPLATE, status=status, label=current_label, collecting_data=collecting_data)

@app.route("/set_label", methods=["POST"])
def set_label():
    global current_label
    new_label = request.form.get("label")
    if new_label in ["awake", "sleep"]:
        current_label = new_label
    return redirect(url_for("index"))

@app.route("/start", methods=["POST"])
def start():
    global collecting_data, data, csv_file
    if not collecting_data:
        collecting_data = True
        data = []
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_file = f"aos_data_{timestamp}.csv"
    return redirect(url_for("index"))

@app.route("/stop", methods=["POST"])
def stop():
    global collecting_data, data, csv_file
    if collecting_data:
        save_data()
        collecting_data = False
        data = []
        csv_file = None
    return redirect(url_for("index"))

@app.route("/data", methods=["POST"])
def receive_data():
    global data, collecting_data, csv_file
    if collecting_data:
        content = request.json
        # print("Received /data batch:", content)
        timestamp = datetime.datetime.now().isoformat()
        
        for sample in content:
            entry = [
                timestamp,
                sample.get("AccelX", 0),
                sample.get("AccelY", 0),
                sample.get("AccelZ", 0),
                sample.get("GyroX", 0),
                sample.get("GyroY", 0),
                sample.get("GyroZ", 0),
                sample.get("IR", 0),
                sample.get("Red", 0),
                current_label
            ]
            data.append(entry)
        
        if len(data) >= SAMPLE_THRESHOLD:
            save_data()
            data = []
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            csv_file = f"aos_data_{timestamp}.csv"
    return jsonify({"status": "success"})

@app.route("/command", methods=["GET"])
def command():
    # print("Received /command request from", request.remote_addr)
    return "start" if collecting_data else "stop"

def save_data():
    global data, csv_file
    if data:
        df = pd.DataFrame(
            data,
            columns=["timestamp", "AccelX", "AccelY", "AccelZ", "GyroX", "GyroY", "GyroZ", "IR", "Red", "label"]
        )
        df.to_csv(csv_file, index=False)
        print(f"Data saved to {csv_file} with {len(data)} samples")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)