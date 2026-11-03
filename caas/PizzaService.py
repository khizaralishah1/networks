import uuid
import socket
import threading
from IdempotencyStore import IdempotencyStore

class PizzaService:
  def __init__(self, replica_id: str, ip_addr: str, port: int, idempotency_store: IdempotencyStore):
    self.replica_id = replica_id
    self.ip_addr = ip_addr
    self.port = int(port)
    self.idempotency_store = idempotency_store # all replicas have access to the shared db
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
          status, response_body, extra_headers = self.handle_request(raw_data)

          http_response = (
            f'HTTP/1.1 {status}\r\n'
            'Content-Type: text/plain\r\n'
            f'Content-Length: {len(response_body)}\r\n'
            + ''.join(f'{name}: {value}\r\n' for name, value in extra_headers.items()) +
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

  def handle_request(self, raw_data: str):
    ''' Returns (status, body, extra_headers) '''
    head, _, request_body = raw_data.partition('\r\n\r\n')
    headers = {}
    for line in head.split('\r\n')[1:]: # skip the request line
      name, _, value = line.partition(':')
      headers[name.strip().lower()] = value.strip()

    key = headers.get('idempotency-key')
    if not key:
      return '400 Bad Request', 'Missing Idempotency-Key header', {}

    record = self.idempotency_store.claim(key, request_body)

    # 1. New key: actually create the order, then remember the response
    if record is None:
      try:
        status, response_body = self.create_order(request_body)
      except Exception:
        self.idempotency_store.release(key)
        raise
      self.idempotency_store.save_response(key, status, response_body)
      return status, response_body, {}

    # 2. Same key, different order: client bug, refuse instead of guessing
    if record["request"] != request_body:
      return '422 Unprocessable Entity', 'Idempotency-Key was already used for a different request', {}

    # 3. Same key, first attempt hasn't finished yet (e.g. client timed out and retried quickly)
    if record["response"] is None:
      return '409 Conflict', 'A request with this Idempotency-Key is still in progress', {}

    # 4. Same key, already done: replay the saved response, don't create a second order
    status, response_body = record["response"]
    return status, response_body, {'Idempotent-Replayed': 'true'}

  def create_order(self, order_text: str):
    unique_order_id = uuid.uuid4()
    self.orders.append((unique_order_id, order_text))
    response_body = (
      f"Order ID: {unique_order_id} | "
      f"Fulfilled by {self.replica_id} at {self.ip_addr}:{self.port}"
    )
    return '200 OK', response_body
