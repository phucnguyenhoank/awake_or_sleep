from scapy.all import ARP, Ether, srp

def scan_network(target_ip):
    # Create an ARP request to check for devices in the network
    arp_request = ARP(pdst=target_ip)
    ether_frame = Ether(dst="ff:ff:ff:ff:ff:ff")
    arp_request_packet = ether_frame/arp_request
    
    # Send the packet and capture the response
    result = srp(arp_request_packet, timeout=3, verbose=False)[0]
    
    devices = []
    for sent, received in result:
        devices.append({'ip': received.psrc, 'mac': received.hwsrc})
    
    return devices

# Scan the network range
devices = scan_network("192.168.75.1/24")

print("Devices found:")
for device in devices:
    print(f"IP: {device['ip']}   MAC: {device['mac']}")
