#include "TCPHelper.h"
#include "TLSHelper.h"

#include <cstring>
#include <iostream>

std::string TCPStuff(const std::string& ip, const std::string& hostname) {
  // 1. Create a TCP Socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    perror("socket");
    exit(1);
  }

  std::cout << "Socket created\n";

  sockaddr_in server{};

  server.sin_family = AF_INET;

  // server.sin_port = htons(80); Simple HTTP. Google respones with redirect. Wants TLS
  server.sin_port = htons(443); // HTTPS, Uses TLS

  inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

  std::cout << "Connecting to " << ip << std::endl;

  if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
    perror("connect");
    exit(1);
  }

  std::cout << "Connected!\n";

  // Handshake is done

  // Send an HTTP request
  std::string response = HttpsRequest(sock, hostname);

  close(sock);

  return response;
}