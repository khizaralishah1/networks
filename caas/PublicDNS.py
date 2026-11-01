import Constants

class PublicDNS:
  def __init__(self):
    self.records = {}

  def add_record(self, domain: str, ip_addr: str, port: int):
    self.records[domain] = (ip_addr, port)

  def resolve(self, domain: str) -> tuple[str, int]:
    if domain not in self.records:
      raise ValueError
    return self.records[domain]
  
  def status(self) -> str:
    return "Alive"

# Runs in the DNS Servers Company
dns = PublicDNS()
dns.add_record("pizza.service.com", Constants.edge_gateway_ip_addr, Constants.edge_gateway_port)