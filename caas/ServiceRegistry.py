class ServiceRegistry:
  def __init__(self):
    self.table = {} # service_name -> endpoints

  def register(self, service_name: str, ip_addr: str, port: int):
    endpoints = self.table.setdefault(service_name, [])
    endpoint = (ip_addr, port)
    if endpoint not in endpoints:
      endpoints.append(endpoint)
      print(f"Service Registry registered {service_name} -> {ip_addr}:{port}")

  def get_endpoints(self, service_name: str): # aka discover
    return list(self.table.get(service_name, []))

  def get_servers(self, service_name):
    return self.table.get(service_name, [])

# you can go from service name to list of end machines
# how do you get the service name?