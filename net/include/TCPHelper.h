#ifndef TCP_HELPER_H
#define TCP_HELPER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <vector>

std::string TCPStuff(const std::string& ip, const std::string& hostname);

#endif // TCP_HELPER_H