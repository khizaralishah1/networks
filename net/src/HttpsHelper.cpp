#include "HttpsHelper.h"

#include <iostream>
#include <sstream>

std::ostream& operator<<(std::ostream& os, const URL& url) {
  os << "Scheme: " << url.scheme << "\r\nHost: " << url.host << "\r\nPath: " << url.path
     << "\r\nPort: " << url.port << "\r\n";

  return os;
}

std::string DecodeChunked(const std::string& body) {
  std::string decoded;
  size_t pos = 0;

  while (true) {
    size_t end = body.find("\r\n", pos);

    std::string size_hex = body.substr(pos, end - pos);
    size_t chunk_size = std::stoul(size_hex, nullptr, 16);  // string to unsigned long
    if (chunk_size == 0) break;
    pos = end + 2;
    decoded.append(body, pos, chunk_size);
    pos += chunk_size + 2;
  }

  return decoded;
}

HttpsResponse ParseHttpsResponse(const std::string& response) {
  std::cout << "############################### PARSE HTTP RESPONSE #########################"
            << std::endl;

  HttpsResponse parsed{};

  // 1. Separate headers and body
  size_t header_end = response.find("\r\n\r\n");

  if (header_end == std::string::npos) {
    std::cerr << "Invalid HTTP response\r\n";
    return parsed;
  }

  std::string header_text = response.substr(0, header_end);
  parsed.body = response.substr(header_end + 4);

  // 2. Parse status line
  std::istringstream header_stream(header_text);
  std::string status_line;
  std::getline(header_stream, status_line);
  if (!status_line.empty() && status_line.back() == '\r') status_line.pop_back();
  std::istringstream status_stream(status_line);
  status_stream >> parsed.version;
  status_stream >> parsed.status_code;
  std::cout << "Version: " << parsed.version << "\r\n";
  std::cout << "Status Code: " << parsed.status_code << "\r\n";

  // Parse Headers
  std::string line;
  while (std::getline(header_stream, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;
    size_t colon = line.find(':');
    if (colon == std::string::npos) continue;

    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 1);

    if (!value.empty() && value.front() == ' ') value.erase(0, 1);

    parsed.headers[key] = value;

    // std::cout << key << ": " << value << "\r\n";
  }

  return parsed;
}

URL ParseURL(const std::string& url) {
  URL result;

  size_t scheme_end = url.find("://");
  result.scheme = url.substr(0, scheme_end);

  size_t host_start = scheme_end + 3;
  size_t path_start = url.find('/', host_start);

  if (path_start == std::string::npos) {
    result.host = url.substr(host_start);
    result.path = "/";
  } else {
    result.host = url.substr(host_start, path_start - host_start);
    result.path = url.substr(path_start);
  }
  result.port = (result.scheme == "https") ? 443 : 80;

  return result;
}

std::string GetHeader(const std::string& response, const std::string& header) {
  std::string search = header + ":";

  size_t pos = response.find(search);
  pos += search.size();
  while (response[pos] == ' ') ++pos;

  size_t end = response.find("\r\n", pos);

  return response.substr(pos, end - pos);
}