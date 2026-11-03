import threading

class IdempotencyStore:
  ''' Shared by ALL replicas. A retry can land on a different replica, so a per-replica dict won't work.
      In real life this is Redis or a database table, not a dict in memory. '''
  def __init__(self):
    self.records = {} # idempotency key -> {"request": body, "response": (status, body) or None}
    self.lock = threading.Lock() # replicas run in parallel, check-and-insert must be atomic

  def claim(self, key: str, request_body: str):
    ''' Returns None if the key is new (caller now owns it), else the existing record '''
    with self.lock:
      if key in self.records:
        return self.records[key]
      self.records[key] = {"request": request_body, "response": None} # None = still in progress
      return None

  def save_response(self, key: str, status: str, body: str):
    with self.lock:
      self.records[key]["response"] = (status, body)

  def release(self, key: str):
    ''' Processing failed, forget the key so the client's retry can try again '''
    with self.lock:
      self.records.pop(key, None)

  # TODO: expire keys after some time (Stripe keeps them 24h), otherwise this grows forever
