from flask import Flask, request, redirect, url_for, render_template_string
import time
from datetime import datetime
import csv
import json

app = Flask(__name__)

# Global state variables
collecting_data = False
data = []  # Buffer to store incoming sensor data
csv_file = None
collection_duration = 0  # Duration in minutes
start_time = 0  # Start time in seconds

# HTML template (your provided template)
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
    return render_template_string(HTML_TEMPLATE, collecting_data=collecting_data, remaining_time=remaining_time)

@app.route("/start", methods=["POST"])
def start():
    global collecting_data, data, csv_file, collection_duration, start_time
    if not collecting_data:
        collecting_data = True
        data = []  # Clear previous data
        collection_duration = int(request.form.get("minutes", 0))
        start_time = time.time()
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_file = f"aos_data_{timestamp}.csv"
    return redirect(url_for("index"))

@app.route("/remaining_time")
def remaining_time():
    return calculate_remaining_time()

@app.route("/is-collecting-data")
def is_collecting_data():
    global collecting_data
    elapsed = time.time() - start_time
    if collecting_data and elapsed >= collection_duration * 60:
        # Time’s up, stop collecting and save data
        collecting_data = False
        save_data_to_csv()
    return "true" if collecting_data else "false"

@app.route("/data", methods=["POST"])
def receive_data():
    global data
    if collecting_data:
        try:
            received_data = json.loads(request.data)
            # Add a timestamp to each data entry
            received_data = [
                {**entry, "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")}
                for entry in received_data
            ]
            data.extend(received_data)  # Add received sensor data to buffer
            return "Data received", 200
        except Exception as e:
            return f"Error processing data: {str(e)}", 400
    return "Not collecting data", 403

def calculate_remaining_time():
    global collecting_data
    if collecting_data:
        elapsed = time.time() - start_time
        remaining = collection_duration * 60 - elapsed
        if remaining > 0:
            minutes, seconds = divmod(int(remaining), 60)
            return f"{minutes}m {seconds}s"
        else:
            collecting_data = False  # Stop collecting when time is up
            save_data_to_csv()
            return "0m 0s"
    return "0m 0s"

def save_data_to_csv():
    global data, csv_file
    if data and csv_file:
        with open(csv_file, "w", newline="") as f:
            writer = csv.writer(f)
            # Write header
            writer.writerow(["Timestamp", "AccelX", "AccelY", "AccelZ", "GyroX", "GyroY", "GyroZ", "IR", "Red"])
            # Write data
            for entry in data:
                writer.writerow([
                    entry["Timestamp"],
                    entry["AccelX"], entry["AccelY"], entry["AccelZ"],
                    entry["GyroX"], entry["GyroY"], entry["GyroZ"],
                    entry["IR"], entry["Red"]
                ])
        print(f"Data saved to {csv_file}")
        data = []  # Clear buffer after saving

if __name__ == "__main__":
    # Replace '0.0.0.0' with your server's IP if needed, and set the port
    app.run(host="0.0.0.0", port=5000, debug=True)