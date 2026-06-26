#include "TLSHelper.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <iostream>

std::string HttpsRequest(int sock, const std::string& hostname) {
  // Initialize OpenSSL
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  // Create client context
  SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    return {};
  }

  // Create SSL object
  SSL* ssl = SSL_new(ctx);
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    SSL_CTX_free(ctx);
    return {};
  }

  SSL_set_tlsext_host_name(ssl, hostname.c_str());

  // Attach socket
  SSL_set_fd(ssl, sock);

  // Perform TLS handshake
  if (SSL_connect(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return {};
  }

  std::cout << "TLS Handshake complete\r\n";
  std::cout << "Protocol: " << SSL_get_version(ssl) << "\r\n";
  std::cout << "Cipher: " << SSL_get_cipher(ssl) << "\r\n";

  // HTTP request
  std::string request = 
    "GET / HTTP/1.1\r\n"
    "Host: " + hostname + "\r\n"
    "Connection: close\r\n"
    "\r\n";

  if (SSL_write(ssl, request.c_str(), request.size()) <= 0) {
    ERR_print_errors_fp(stderr);
  }

  std::cout << "Sent: " << request << std::endl;

  // Read response
  char buffer[4096];
  std::string response;
  std::cout << "################################### HTTP RESPONSE ######################\r\n";

  while(true) {
    int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytes <=0) break;
    buffer[bytes] = '\0';
    response += buffer;
  }

  // Shutdown TLS
  SSL_shutdown(ssl);
  SSL_free(ssl);
  SSL_CTX_free(ctx);

  return response;
}