#include <cstring>
#include <iostream>
#include <vector>

#include "DNSHelper.h"

int main() {
  std::string hostname = "google.com";
  std::string ip = ResolveDNS(hostname);

  return EXIT_SUCCESS;
}