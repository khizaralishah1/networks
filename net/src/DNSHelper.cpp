#include "DNSHelper.h"

#include <iostream>

void encodeDomain(const std::string& domain, std::vector<uint8_t>& packet) {
  size_t start = 0;

  while (true) {
    size_t dot = domain.find('.', start);

    std::string label =
        (dot == std::string::npos) ? domain.substr(start) : domain.substr(start, dot - start);

    packet.push_back(label.size());

    for (char c : label) packet.push_back(c);

    if (dot == std::string::npos) break;

    start = dot + 1;
  }

  packet.push_back(0);

  // Further
  // 4. Append q type
  uint16_t qtype = htons(1);
  packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&qtype),
                reinterpret_cast<uint8_t*>(&qtype) + sizeof(qtype));

  // 5. Append q class
  uint16_t qclass = htons(1);
  packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&qclass),
                reinterpret_cast<uint8_t*>(&qclass) + sizeof(qclass));
}

void ParseDNSResponse(uint8_t* buffer, ssize_t n, std::string& ip_) {
  std::cout << "Received " << n << " bytes\n";
  std::cout << "Raw DNS Response:\n";

  for (ssize_t i = 0; i < n; i++) {
    printf("%02X ", buffer[i]);

    // Print 16 bytes per line
    if ((i + 1) % 16 == 0) printf("\n");
  }

  printf("\n");

  // ===========================
  // Parse DNS Header
  // ===========================
  DNSHeader* response = reinterpret_cast<DNSHeader*>(buffer);

  std::cout << "\n===== DNS HEADER =====\n";
  std::cout << "Transaction ID : " << ntohs(response->id) << '\n';
  std::cout << "Flags          : 0x" << std::hex << ntohs(response->flags) << std::dec << '\n';
  std::cout << "Questions      : " << ntohs(response->qdcount) << '\n';
  std::cout << "Answers        : " << ntohs(response->ancount) << '\n';
  std::cout << "Authority RRs  : " << ntohs(response->nscount) << '\n';
  std::cout << "Additional RRs : " << ntohs(response->arcount) << '\n';

  // ===========================
  // Parse Question Section
  // ===========================
  size_t offset = sizeof(DNSHeader);

  std::cout << "\n===== QUESTION =====\n";

  while (buffer[offset] != 0) {
    uint8_t labelLength = buffer[offset++];

    std::cout << "Label (" << (int)labelLength << "): ";

    for (int i = 0; i < labelLength; i++) {
      std::cout << static_cast<char>(buffer[offset++]);
    }

    std::cout << '\n';
  }

  // Skip terminating zero
  offset++;

  // Read QTYPE
  uint16_t responseQType = ntohs(*reinterpret_cast<uint16_t*>(buffer + offset));
  offset += 2;

  // Read QCLASS
  uint16_t responseQClass = ntohs(*reinterpret_cast<uint16_t*>(buffer + offset));
  offset += 2;

  std::cout << "QTYPE  : " << responseQType << '\n';
  std::cout << "QCLASS : " << responseQClass << '\n';
  std::cout << "\nAnswer starts at byte offset " << offset << '\n';

  std::cout << "\n===== ANSWER =====\n";

  uint16_t name = ntohs(*reinterpret_cast<uint16_t*>(buffer + offset));

  std::cout << "NAME Pointer: 0x" << std::hex << name << std::dec << '\n';

  offset += 2;

  uint16_t answerType = ntohs(*reinterpret_cast<uint16_t*>(buffer + offset));

  offset += 2;

  std::cout << "TYPE: " << answerType << '\n';

  uint16_t answerClass = ntohs(*reinterpret_cast<uint16_t*>(buffer + offset));

  offset += 2;

  std::cout << "CLASS: " << answerClass << '\n';

  uint32_t ttl = ntohl(*reinterpret_cast<uint32_t*>(buffer + offset));

  offset += 4;

  std::cout << "TTL: " << ttl << " seconds\n";

  uint16_t rdLength = ntohs(*reinterpret_cast<uint16_t*>(buffer + offset));

  offset += 2;

  std::cout << "Data Length: " << rdLength << '\n';

  char ip[INET_ADDRSTRLEN];

  inet_ntop(AF_INET, buffer + offset, ip, sizeof(ip));

  std::cout << "IPv4 Address: " << ip << '\n';

  for (int i = 0; i < INET_ADDRSTRLEN; ++i) {
    ip_[i] = ip[i];
  }
}

std::string ResolveDNS(const std::string& domain) {
  // 1. Header
  DNSHeader header{};
  header.id = htons(1234);
  header.flags = htons(0x0100);
  header.qdcount = htons(1);

  // 2. DNS Packet
  std::vector<uint8_t> packet;
  packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&header),
                reinterpret_cast<uint8_t*>(&header) + sizeof(header));
  std::cout << "Header size: " << sizeof(header) << '\n';

  // 3. Encode domain
  encodeDomain(domain, packet);

  std::cout << "Packet size: " << packet.size() << '\n';

  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock < 0) {
    perror("socket");
    exit(1);
  }

  sockaddr_in server{};
  server.sin_family = AF_INET;
  server.sin_port = htons(53);

  inet_pton(AF_INET, "8.8.8.8", &server.sin_addr);

  std::cout << "Packet bytes:\n";
  for (uint8_t b : packet) {
    printf("%02X ", b);
  }
  std::cout << '\n';

  // Send packet
  ssize_t sent = sendto(sock, packet.data(), packet.size(), 0, (sockaddr*)&server, sizeof(server));
  if (sent < 0) {
    perror("sendto");
    exit(1);
  }

  std::cout << "Sent " << sent << " bytes\n";

  // Response
  uint8_t buffer[512];

  ssize_t n = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
  if (n < 0) {
    perror("recvfrom");
    exit(1);
  }
  close(sock);

  std::string ip;
  ParseDNSResponse(buffer, n, ip);

  return ip;
}