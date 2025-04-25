import socket
import json
import csv
import datetime

UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 5001

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
print(f"UDP server up and listening on port {UDP_PORT}")

# Global dataset and label (the label can be updated from another interface if desired)
dataset = []
label = "awake"  # Default label

CSV_FILENAME = "sensor_data.csv"

def save_to_csv():
    with open(CSV_FILENAME, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["SavedTime", "Timestamp", "Label", "AccelX", "AccelY", "AccelZ",
                         "GyroX", "GyroY", "GyroZ", "IR"])
        for entry in dataset:
            writer.writerow([
                entry["SavedTime"], entry["Timestamp"], entry["Label"],
                entry["AccelX"], entry["AccelY"], entry["AccelZ"],
                entry["GyroX"], entry["GyroY"], entry["GyroZ"],
                entry["IR"]
            ])
    print(f"Data saved to {CSV_FILENAME}")

print("Waiting to receive data...")

while True:
    data, addr = sock.recvfrom(8192)  # Buffer size (in bytes)
    try:
        # Parse JSON data received from the ESP32
        sensor_entries = json.loads(data.decode('utf-8'))
        now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
        for entry in sensor_entries:
            entry["SavedTime"] = now
            entry["Label"] = label  # Attach the current label
            dataset.append(entry)
        print(f"Received {len(sensor_entries)} entries from {addr}")
        
        # Optionally save to CSV after a certain number of entries
        if len(dataset) >= 400:
            save_to_csv()
            dataset = []
    except Exception as e:
        print("Error processing data:", e)
