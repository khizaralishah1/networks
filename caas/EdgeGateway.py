import Constants
import socket
import threading
from ClusterManager import registry
import random

class EdgeGateway:
  def __init__(self, service_registry, ip_addr: str, port: int = 8080):
    self.registry = service_registry
    self.ip_addr = ip_addr
    self.port = port
    self.routes = {}
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    self.socket.bind((self.ip_addr, self.port))

  def add_route(self, domain: str, internal_service_name: str):
    ''' Infrastructure team does this '''
    self.routes[domain] = internal_service_name

  def start(self):
    self.socket.listen(128)
    print(f"Edge gateway listening on {self.ip_addr}:{self.port}...")
    self.listen_loop()

  def listen_loop(self):
    try:
      # runs forever, one thread per client so a slow client doesn't block others
      while True:
        conn, addr = self.socket.accept()
        threading.Thread(target=self.handle_connection, args=(conn,), daemon=True).start()
    finally:
      self.socket.close()

  def handle_connection(self, conn: socket.socket):
    try:
      raw_data = conn.recv(2048).decode('utf-8')
      # print('[EDGE GATEWAY RECEIVED RAW TCP PACKET]')
      # print(raw_data.strip())

      # parse to find host header
      host_header = ''
      for line in raw_data.split('\r\n'): 
        if line.lower().startswith('host:'): 
          host_header = line.split(':', 1)[1].strip()
          break
      # print(f"Gateway extracted Host Header: {host_header}")

      if host_header in self.routes:
        internal_target = self.routes[host_header]
        # print(f"Routing rule matched: {host_header} to service {internal_target}")
        
        # Query the service registry
        endpoints = self.registry.get_endpoints(internal_target)
        # print(f"Endpoints: {endpoints}")

        # Choose any endpoint. TODO: load-balancing
        target_ip, target_port = random.choice(endpoints)
        target_port = int(target_port)

        # Open a temporary internal tcp connection to that replica
        backend_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
          backend_socket.connect((target_ip, target_port))
          backend_socket.sendall(raw_data.encode('utf-8')) # forward
          backend_response = backend_socket.recv(4096)
          # print(f"Backend response: {backend_response.strip()}")
          conn.sendall(backend_response) # to client
        except Exception as e:
          print(f"Failed to communicate with replica {target_ip}:{target_port} -> {e}")
        finally:
          backend_socket.close()
    except Exception as e:
      print(f"Edge gateway can't handle connect")
    finally:
      conn.close()

if __name__ == '__main__':
  edge_gateway = EdgeGateway(registry, Constants.edge_gateway_ip_addr, Constants.edge_gateway_port)
  edge_gateway.add_route("pizza.service.com", "pizza-service")

  # Runs until stopped with Ctrl+C
  try:
    edge_gateway.start()
  except KeyboardInterrupt:
    print("\nEdge gateway shutting down")
    