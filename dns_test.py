from flask import Flask
from zeroconf import ServiceInfo, Zeroconf
import socket

app = Flask(__name__)

@app.route('/')
def home():
    return "Hello from Flask!"

if __name__ == "__main__":
    ip = socket.gethostbyname(socket.gethostname())
    info = ServiceInfo(
        "_http._tcp.local.",
        "xichxo._http._tcp.local.",
        addresses=[socket.inet_aton(ip)],
        port=5000,
        properties={},
    )
    zeroconf = Zeroconf()
    zeroconf.register_service(info)
    try:
        app.run(host="0.0.0.0", port=5000)
    finally:
        zeroconf.unregister_service(info)
        zeroconf.close()