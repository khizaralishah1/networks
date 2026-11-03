import json
import socket
import time
import uuid

class RealPizzaClient:
  def __init__(self, domain: str, ip_addr: str, port: int):
    self.domain = domain
    self.ip_addr = ip_addr
    self.port = int(port)

  def order_pizza(self, pizza_type: str, quantity: int, idempotency_key: str = None, max_attempts: int = 3):
    # One key per logical order. Every retry of THIS order reuses it, so the server can spot duplicates
    idempotency_key = idempotency_key or str(uuid.uuid4())

    payload = json.dumps({"pizza": pizza_type, "quantity": quantity})
    http_request = (
      f"POST /v1/orders HTTP/1.1\r\n"
      f"Host: {self.domain}\r\n"
      f"User-Agent: RealPizzaClient/1.0\r\n"
      f"Content-Type: application/json\r\n"
      f"Content-Length: {len(payload)}\r\n"
      f"Idempotency-Key: {idempotency_key}\r\n"
      f"Connection: close\r\n\r\n"
      f"{payload}"
    )

    for attempt in range(1, max_attempts + 1):
      try:
        response_text = self.send(http_request) # this is send+recv
        break
      except OSError as e:
        # We don't know if the order was created or not. Safe to retry ONLY because the key is the same
        print(f"Attempt {attempt} failed ({e}), retrying with same Idempotency-Key")
        if attempt == max_attempts:
          raise
        time.sleep(0.1 * 2 ** attempt) # back off a bit before retrying

    if "\r\n\r\n" in response_text:
        headers, response_body = response_text.split("\r\n\r\n", 1)
    else:
        headers, response_body = "", response_text # Fallback if response is malformed

    replayed = "idempotent-replayed: true" in headers.lower()
    print(("[replayed] " if replayed else "") + response_body.strip())
    return response_text

  def send(self, http_request: str) -> str:
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

    # Gateway closes without replying when the replica fails, treat it like a network error
    if not response_bytes:
      raise ConnectionError("empty response")
    return response_bytes.decode('utf-8')
