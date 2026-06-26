#ifndef DNS_HELPER_H
#define DNS_HELPER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <vector>

struct DNSHeader {
  uint16_t id;
  uint16_t flags;
  uint16_t qdcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
};

void encodeDomain(const std::string& domain, std::vector<uint8_t>& packet);
void ParseDNSResponse(uint8_t* buffer, ssize_t n, std::string& ip);
std::string ResolveDNS(const std::string& domain);

#endif // DNS_HELPER_H