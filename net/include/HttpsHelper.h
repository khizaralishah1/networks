#ifndef HTTPS_HELPER_H
#define HTTPS_HELPER_H

#include <string>
#include <map>

struct URL {
  std::string scheme;
  std::string host;
  std::string path;
  int port;
};

struct HttpsResponse {
  int status_code;
  std::string version;
  std::unordered_map<std::string, std::string> headers;

  std::string body;
};

std::ostream& operator<<(std::ostream& os, const URL& url);

std::string DecodeChunked(const std::string& body);
HttpsResponse ParseHttpsResponse(const std::string& response);
std::string GetHeader(const std::string& response, const std::string& header);
URL ParseURL(const std::string& url);

#endif  // HTPPS_HELPER_H