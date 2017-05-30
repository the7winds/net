#include "tcp_socket.h"
#include "util.h"
#include <unistd.h>
#include <iostream>


/*
 * tcp_socket implementation
 */

void tcp_connection_socket::send(const void *buf, size_t size) {
    size_t sended;

    while (size) {
        sended = ::send(sk, buf, size, 0);
        if (sended < 0)
            throw std::runtime_error("can't send all data");
        size -= sended;
	buf = (char*) buf + sended;
    }
}


void tcp_connection_socket::recv(void *buf, size_t size) {
    size_t recved;

    while (size) {
        recved = ::recv(sk, buf, size, 0);
        if (recved < 0)
            throw std::runtime_error("can't receive all data");
	size -= recved;
        buf = (char*) buf + recved;
    }
}


void tcp_connection_socket::close() {
    ::shutdown(sk, SHUT_RDWR);
    ::close(sk);
    closed = true;
}


tcp_connection_socket::~tcp_connection_socket() {
    if (!closed)
        ::close(sk);
}


/*
 * tcp_client_socket implementation
 */

tcp_client_socket::tcp_client_socket(const char *addr, tcp_port port) {
    sk.sk = socket(AF_INET, SOCK_STREAM, 0);
    init_ipv4addr(addr, port, ipv4addr);
}


void tcp_client_socket::connect() {
    if (connected) return;

    if (sk.sk < 0)
        throw std::runtime_error("invalid socket descriptor");

    if (::connect(sk.sk, (sockaddr *) &ipv4addr, sizeof(ipv4addr)) < 0)
        throw std::runtime_error("can't connect");

    connected = true;
}


void tcp_client_socket::send(const void *buf, size_t size) {
    sk.send(buf, size);
}


void tcp_client_socket::recv(void *buf, size_t size) {
    sk.recv(buf, size);
}


tcp_client_socket::~tcp_client_socket() {
    close(sk.sk);
}


/*
 * tcp_server_socket implementation
 */

tcp_server_socket::tcp_server_socket(const char *addr, tcp_port port) {
    sk = socket(AF_INET, SOCK_STREAM, 0);
    init_ipv4addr(addr, port, ipv4addr);

    int optval = 1;
    if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, (void *) &optval, sizeof(int)) < 0)
        throw std::runtime_error("can't set SO_REUSEADDR");

    if (bind(sk, (const sockaddr *) &ipv4addr, sizeof(ipv4addr)) < 0)
        throw std::runtime_error("can't bind socket");

    if (listen(sk, BACKLOG) < 0)
        throw std::runtime_error("can't listen");
}


stream_socket *tcp_server_socket::accept_one_client() {
    socklen_t addrLen = sizeof(ipv4addr);
    int client_sk = accept(sk, (sockaddr *) &ipv4addr, &addrLen);

    if (client_sk < 0)
        throw std::runtime_error("can't accept socket");

    return (stream_socket*) new tcp_connection_socket(client_sk);
}

void tcp_server_socket::close() {
    if (sk >= 0) {
        ::shutdown(sk, SHUT_RDWR);
        ::close(sk);
    }

    std::cerr << "server socket is closed\n";
}

tcp_server_socket::~tcp_server_socket() {
    if (sk >= 0)
        close();
}
