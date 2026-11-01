import uuid
import socket
import threading

class PizzaService:
  def __init__(self, replica_id: str, ip_addr: str, port: int):
    self.replica_id = replica_id
    self.ip_addr = ip_addr
    self.port = int(port)
    self.orders = []
    self.server_socket = None
    self.start()

  def start(self):
    self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    self.server_socket.bind((self.ip_addr, self.port))
    self.server_socket.listen(5)
    print(f"Pizza service running at {self.ip_addr}:{self.port}")

    def listen_loop():
      while True:
        conn, addr = self.server_socket.accept()
        try:
          raw_data = conn.recv(2048).decode("utf-8")
          unique_order_id = uuid.uuid4()
          response_body = (
            f"Order ID: {unique_order_id} | "
            f"Fulfilled by {self.replica_id} at {self.ip_addr}:{self.port}"
          )

          http_response = (
            'HTTP/1.1 200 OK\r\n'
            'Content-Type: text/plain\r\n'
            f'Content-Length: {len(response_body)}\r\n'
            'Connection: close\r\n'
            '\r\n' + response_body
          )
          conn.sendall(http_response.encode('utf-8'))
        except Exception as e:
          print(f"Error handling request on replica {self.replica_id}: {e}")
        finally:
          conn.close()
    # start function runs the thread
    threading.Thread(target=listen_loop, daemon=True).start()

  def handle_order(self, order_text):
    self.orders.append(order_text)
    return f"Order confirmed: '{order_text}' by {self.name}"

