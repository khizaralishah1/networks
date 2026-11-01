import json
import socket

class RealPizzaClient:
  def __init__(self, domain: str, ip_addr: str, port: int):
    self.domain = domain
    self.ip_addr = ip_addr
    self.port = int(port)

  def order_pizza(self, pizza_type: str, quantity: int):
    payload = json.dumps({"pizza": pizza_type, "quantity": quantity})
    http_request = (
      f"POST /v1/orders HTTP/1.1\r\n"
      f"Host: {self.domain}\r\n"
      f"User-Agent: RealPizzaClient/1.0\r\n"
      f"Content-Type: application/json\r\n"
      f"Content-Length: {len(payload)}\r\n"
      f"Connection: close\r\n\r\n"
      f"{payload}"
    )

    self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.client_socket.connect((self.ip_addr, self.port))
    self.client_socket.sendall(http_request.encode("utf-8"))
    # print("client send http request to edge gateway")
    response_bytes = b""
    while True:
      chunk = self.client_socket.recv(1024)
      if not chunk:
        break
      response_bytes += chunk
    self.client_socket.close()

    response_text = response_bytes.decode('utf-8')
    if "\r\n\r\n" in response_text:
        headers, response_body = response_text.split("\r\n\r\n", 1)
    else:
        response_body = response_text # Fallback if response is malformed

    print(response_body.strip())
    return response_text