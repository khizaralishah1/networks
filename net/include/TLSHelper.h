#ifndef TLS_HELPER_H
#define TLS_HELPER_H

#include <string>

std::string HttpsRequest(int sock, const std::string& hostname);

#endif // TLS_HELPER_H