#pragma once

#include "stream_socket.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <exception>
#include <netinet/in.h>
#include <list>

class tcp_connection_socket;

class tcp_server_socket : public stream_server_socket {
    const static int BACKLOG = 5;
    int sk = -1;
    sockaddr_in ipv4addr;

public:
    tcp_server_socket(const char *addr, uint16_t port);

    stream_socket *accept_one_client() override;

    void close();

    ~tcp_server_socket() override;
};


class tcp_connection_socket : public stream_socket {
    int sk;
    bool closed = false;

    tcp_connection_socket() {}
    tcp_connection_socket(int sk) : sk(sk) {};

    friend stream_socket *tcp_server_socket::accept_one_client();
    friend class tcp_client_socket;

public:
    void send(const void *buf, size_t size) override;

    void recv(void *buf, size_t size) override;

    void close();

    ~tcp_connection_socket();
};


class tcp_client_socket : public stream_client_socket {
    tcp_connection_socket sk;
    sockaddr_in ipv4addr;
    bool connected = false;

public:
    tcp_client_socket(const char *addr, uint16_t port);

    void send(const void *buf, size_t size) override;

    void recv(void *buf, size_t size) override;

    void connect() override;

    ~tcp_client_socket() override;
};
