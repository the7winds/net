#include <netinet/in.h>
#include <memory>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include "stream_socket.h"
#include "util.h"


void init_ipv4addr(const char *addr, tcp_port port, sockaddr_in &ipv4addr) {
    ipv4addr.sin_family = AF_INET;
    ipv4addr.sin_port = htons(port);
    addrinfo *info;
    if (getaddrinfo(addr, NULL, NULL, &info))
        throw std::runtime_error("can't create ipv4addr");

    ipv4addr.sin_addr = ((sockaddr_in*) info->ai_addr)->sin_addr;
    freeaddrinfo(info);
}


void send_string(std::string &str, stream_socket *sk) {
    uint32_t t32;

    const char *cstr = str.c_str();

    t32 = htonl((uint32_t) (str.length() + 1));
    sk->send(&t32, sizeof(t32));

    sk->send(cstr, str.length() + 1);
}


std::string recv_string(stream_socket *sk) {
    uint32_t t32;

    sk->recv(&t32, sizeof(t32));
    uint32_t descriptionLen = ntohl(t32);

    std::unique_ptr<char> buf(new char[descriptionLen]);
    sk->recv(buf.get(), descriptionLen);

    return std::string(buf.get());
}


void send_uint(uint32_t x, stream_socket *sk) {
    uint32_t t = htonl(x);
    sk->send(&t, sizeof(t));
}


void recv_uint(uint32_t &x, stream_socket *sk) {
    sk->recv(&x, sizeof(x));
    x = ntohl(x);
}


void send_bool(bool x, stream_socket *sk) {
    uint8_t t8 = (uint8_t) x;
    sk->send(&t8, sizeof(t8));
}


void recv_bool(bool &x, stream_socket *sk) {
    uint8_t t8 = (uint8_t) x;
    sk->recv(&t8, sizeof(t8));
    x = t8;
}
