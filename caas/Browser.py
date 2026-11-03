import Constants
import time
import uuid
# from EdgeGateway import EdgeGateway
from PublicDNS import dns
from RealPizzaClient import RealPizzaClient

domain_name = "pizza.service.com"

print(f"Public DNS status: {dns.status()}")

resolved_ip, resolved_port = dns.resolve(domain_name)
print(f"Browser resolved {domain_name} -> {resolved_ip}:{resolved_port}")

client = RealPizzaClient(domain_name, resolved_ip, resolved_port)

# Idempotency demo: pretend the first response got lost, so we send the same order again with the same key.
# Expect the SAME Order ID twice, the second one marked [replayed], even if it hits a different replica
key = str(uuid.uuid4())
client.order_pizza("margherita", 2, idempotency_key=key)
client.order_pizza("margherita", 2, idempotency_key=key)

# for i in range(0, 500):
#   client.order_pizza("margherita", 2)
