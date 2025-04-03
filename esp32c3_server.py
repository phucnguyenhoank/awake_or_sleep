from flask import Flask, request, redirect, url_for, render_template_string
import time
from datetime import datetime
import csv
import json

app = Flask(__name__)

# Global state variables
collecting_data = False
data = []
csv_file = None
collection_duration = 0
start_time = 0
label = "awake"  # Default label

# Updated HTML template with dropdown
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Data Collection</title>
</head>
<body>
    <h1>Data Collection</h1>
    <form action="/start" method="post">
        <input type="number" name="minutes" min="1" placeholder="Minutes" required>
        <button type="submit" {% if collecting_data %}disabled{% endif %}>Start</button>
    </form>
    <!-- Dropdown to choose label -->
    <form action="/set_label" method="post" class="form">
      <label for="label">Select Label:</label>
      <select name="label" id="label" onchange="this.form.submit()">
        <option value="awake" {% if label == 'awake' %}selected{% endif %}>Awake</option>
        <option value="sleep" {% if label == 'sleep' %}selected{% endif %}>Sleep</option>
      </select>
    </form>
    <p>Remaining Time: <span id="remaining_time">{{ remaining_time }}</span></p>
    <script>
        function updateRemainingTime() {
            var xhr = new XMLHttpRequest();
            xhr.open('GET', '/remaining_time', true);
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4 && xhr.status == 200) {
                    document.getElementById('remaining_time').innerHTML = xhr.responseText;
                    var btn = document.querySelector('button');
                    if (xhr.responseText === "0m 0s") {
                        btn.disabled = false;
                    }
                }
            };
            xhr.send();
        }
        setInterval(updateRemainingTime, 1000);
    </script>
</body>
</html>
"""

@app.route("/")
def index():
    remaining_time = calculate_remaining_time()
    return render_template_string(HTML_TEMPLATE, collecting_data=collecting_data, remaining_time=remaining_time, label=label)

@app.route("/start", methods=["POST"])
def start():
    global collecting_data, data, collection_duration, start_time
    if not collecting_data:
        collecting_data = True
        data = []
        collection_duration = int(request.form.get("minutes", 0))
        start_time = time.time()
    return redirect(url_for("index"))

@app.route("/remaining_time")
def remaining_time():
    return calculate_remaining_time()

@app.route("/is_collecting_data")
def is_collecting_data():
    global collecting_data
    elapsed = time.time() - start_time
    if collecting_data and elapsed >= collection_duration * 60:
        collecting_data = False
    return "true" if collecting_data else "false"

@app.route("/set_label", methods=["POST"])
def set_label():
    global label
    label = request.form.get("label", "awake")  # Default to "awake" if no label provided
    return redirect(url_for("index"))

@app.route("/data", methods=["POST"])
def receive_data():
    global data
    try:
        received_data = request.get_json(force=True)  # Force parsing even if content-type is off
        # print("Parsed data (first 5 entries):", received_data[:5])  # First 5 for brevity
        timestamped_data = [
            {**entry, "SavedTime": datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f"), "Label": label}
            for entry in received_data
        ]
        data.extend(timestamped_data)
        save_data_to_csv()  # Uncomment if you want to save every POST
        return "Data received", 200
    except Exception as e:
        print("Error details:", str(e))
        return f"Error processing data: {str(e)}", 400

def calculate_remaining_time():
    global collecting_data
    if collecting_data:
        elapsed = time.time() - start_time
        remaining = collection_duration * 60 - elapsed
        if remaining > 0:
            minutes, seconds = divmod(int(remaining), 60)
            return f"{minutes}m {seconds}s"
        else:
            collecting_data = False
            return "0m 0s"
    return "0m 0s"

def save_data_to_csv():
    global data, csv_file
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = f"./aos_data/phuc_{timestamp}.csv"
    if data and csv_file:
        with open(csv_file, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["SavedTime", "Timestamp", "Label", "AccelX", "AccelY", "AccelZ", "GyroX", "GyroY", "GyroZ", "IR"])
            for entry in data:
                writer.writerow([
                    entry["SavedTime"], entry["Timestamp"], entry["Label"],
                    entry["AccelX"], entry["AccelY"], entry["AccelZ"],
                    entry["GyroX"], entry["GyroY"], entry["GyroZ"],
                    entry["IR"]
                ])
        print(f"Data saved to {csv_file}")
        data = []

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)