loopback_ip_addr = "127.0.0.1"

# Info for cluster manager. When new machine is installed into the data center, you add it
# TODO: Automate that as well
# Let 0th index be yourself (cluster)
machines_info = [[loopback_ip_addr, "1010"], [loopback_ip_addr, "8080"], [loopback_ip_addr, "7070"], [loopback_ip_addr, "6060"]]


edge_gateway_ip_addr = loopback_ip_addr
edge_gateway_port = 9876
