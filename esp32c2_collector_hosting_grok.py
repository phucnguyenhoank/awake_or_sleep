from flask import Flask, request, jsonify, render_template_string, redirect, url_for
import pandas as pd
import datetime

app = Flask(__name__)

# Global variables
collecting_data = False
data = []
csv_file = None
current_label = "awake"  # Default label

# HTML Template for the web interface
HTML_TEMPLATE = """
<!doctype html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-C3 Data Collection</title>
  <link rel="stylesheet" href="/static/style.css">
</head>
<body>
  <div class="container">
    <h1>ESP32-C3 Data Collection</h1>
    <p>Status: <strong>{{ status }}</strong></p>
    <p>Current Label: <strong>{{ label }}</strong></p>
    
    <!-- Dropdown to choose label -->
    <form action="/set_label" method="post" class="form">
      <label for="label">Select Label:</label>
      <select name="label" id="label" onchange="this.form.submit()">
        <option value="awake" {% if label == 'awake' %}selected{% endif %}>Awake</option>
        <option value="sleep" {% if label == 'sleep' %}selected{% endif %}>Sleep</option>
      </select>
    </form>
    
    <!-- Start/Stop buttons -->
    <div class="button-group">
      <form action="/start" method="post">
        <button type="submit" class="btn">Start</button>
      </form>
      <form action="/stop" method="post">
        <button type="submit" class="btn">Stop</button>
      </form>
    </div>
  </div>
</body>
</html>
"""

# Route to display the web interface
@app.route("/")
def index():
    status = "Collecting Data" if collecting_data else "Idle"
    return render_template_string(HTML_TEMPLATE, status=status, label=current_label)

# Route to update the label
@app.route("/set_label", methods=["POST"])
def set_label():
    global current_label
    new_label = request.form.get("label")
    if new_label in ["awake", "sleep"]:
        current_label = new_label
    return redirect(url_for("index"))

# Route to start collecting data
@app.route("/start", methods=["POST"])
def start():
    global collecting_data, data, csv_file
    if not collecting_data:
        collecting_data = True
        data = []
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_file = f"mpu6050_max30102_data_{timestamp}.csv"
    return redirect(url_for("index"))

# Route to stop collecting data and save CSV
@app.route("/stop", methods=["POST"])
def stop():
    global collecting_data, data, csv_file
    if collecting_data:
        collecting_data = False
        # Save data to CSV with updated columns
        df = pd.DataFrame(
            data, 
            columns=["timestamp", "AccelX", "AccelY", "AccelZ",
                     "GyroX", "GyroY", "GyroZ", "IR", "Red", "label"]
        )
        df.to_csv(csv_file, index=False)
        print(f"Data saved to {csv_file}")
    return redirect(url_for("index"))

# Route to receive data from ESP32-C3
@app.route("/data", methods=["POST"])
def receive_data():
    global data
    if collecting_data:
        content = request.json
        timestamp = datetime.datetime.now().isoformat()
        entry = [
            timestamp,
            content.get("AccelX", 0),
            content.get("AccelY", 0),
            content.get("AccelZ", 0),
            content.get("GyroX", 0),
            content.get("GyroY", 0),
            content.get("GyroZ", 0),
            content.get("IR", 0),
            content.get("Red", 0),
            current_label  # Add the current label to the record
        ]
        data.append(entry)
    return jsonify({"status": "success"})

# ESP32 checks this route for commands
@app.route("/command", methods=["GET"])
def command():
    return "start" if collecting_data else "stop"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)