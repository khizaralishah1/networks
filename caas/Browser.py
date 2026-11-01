import Constants
import time
# from EdgeGateway import EdgeGateway
from PublicDNS import dns
from RealPizzaClient import RealPizzaClient

domain_name = "pizza.service.com"

print(f"Public DNS status: {dns.status()}")

resolved_ip, resolved_port = dns.resolve(domain_name)
print(f"Browser resolved {domain_name} -> {resolved_ip}:{resolved_port}")

client = RealPizzaClient(domain_name, resolved_ip, resolved_port)

for i in range(0, 500):
  client.order_pizza("margherita", 2)
