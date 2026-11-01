
        response_body = f"Routed successfully to {internal_target}"
        http_response = (
          'HTTP/1.1 200 OK\r\n'
          'Content-Type: text/plain\r\n'
          f'Content-Length: {len(response_body)}\r\n'
          'Connection: close\r\n'
          '\r\n' + response_body
        )
      else:
        http_response = 'HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n'

      # send back to client
      conn.sendall(http_response.encode('utf-8'))