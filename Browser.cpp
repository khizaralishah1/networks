#include <iostream>

#include "DNSHelper.h"
#include "HttpsHelper.h"
#include "TCPHelper.h"

int main() {
  // 0. Get IP (DNS Client)
  std::cout << "################################ DNS ################################\r\n";
  std::string hostname = "google.com";
  std::string ip = ResolveDNS(hostname);

  // 1. TCP Stuff
  std::cout << "################################ TCP ################################\r\n";
  std::string raw_response = TCPStuff(ip, hostname);
  std::cout << raw_response << std::endl;

  // Parse response
  HttpsResponse response = ParseHttpsResponse(raw_response);
  std::cout << "RESPONSE BODY: " << response.body << std::endl;

  if (response.status_code == 301 || response.status_code == 302) {
    URL url = ParseURL(response.headers["Location"]);
    std::cout << "URL: " << url << "\r\n";
    std::cout << "################################ DNS 2 ################################\r\n";
    ip = ResolveDNS(url.host);
    std::cout << "################################ TCP ################################\r\n";
    std::string response2 = TCPStuff(ip, url.host);
    std::cout << "RESPONSE 2: " << std::endl;
    // std::cout << response2 << std::endl;
    response = ParseHttpsResponse(response2);
  }

  std::cout << "Encoding: " << response.headers["Transfer-Encoding"] << std::endl;

  if (response.headers["Transfer-Encoding"] == "chunked") {
    std::cout << "################################ TRANSFER ENCODING "
                 "################################\r\n";
    response.body = DecodeChunked(response.body);
    std::cout << "DECODED HTML BODY: \r\n" << response.body.substr(0, 100) << std::endl;
  }

  return EXIT_SUCCESS;
}