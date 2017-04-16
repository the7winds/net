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
        sended = ::send(sk, buf, size, 0)) {
        if (sended < 0) {
            err_msg = "can't send all data";
            perror(err_msg);
            throw std::runtime_error(err_msg);
	}
        size -= sended;
	buf += sended;
    }
}


void tcp_connection_socket::recv(void *buf, size_t size) {
    size_t recved;

    while (size) {
        recved = ::recv(sk, buf, size, 0);
        if (recved < 0) {
            err_msg = "can't receive all data";
            perror(err_msg);
            throw std::runtime_error(err_msg);
        }
	size -= recved;
        buf += recved;
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

    if (err_msg)
        throw std::runtime_error(err_msg);

    if (sk.sk < 0) {
        err_msg = "invalid socket descriptor";
        perror(err_msg);
        throw std::runtime_error(err_msg);
    }

    if (::connect(sk.sk, (sockaddr *) &ipv4addr, sizeof(ipv4addr)) < 0) {
        err_msg = "can't connect";
        perror(err_msg);
        throw std::runtime_error(err_msg);
    }

    connected = true;
}


void tcp_client_socket::send(const void *buf, size_t size) {
    sk.send(buf, size);
}


void tcp_client_socket::recv(void *buf, size_t size) {
    sk.recv(buf, size);
}


tcp_client_socket::~tcp_client_socket() {
    close(sk);
}


/*
 * tcp_server_socket implementation
 */

tcp_server_socket::tcp_server_socket(const char *addr, tcp_port port) {
    sk = socket(AF_INET, SOCK_STREAM, 0);
    init_ipv4addr(addr, port, ipv4addr);
    err_msg = nullptr;

    int optval = 1;
    if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, (void *) &optval, sizeof(int)) < 0) {
        err_msg = "can't set SO_REUSEADDR";
        perror(err_msg);
        return;
    }

    if (bind(sk, (const sockaddr *) &ipv4addr, sizeof(ipv4addr)) < 0) {
        err_msg = "can't bind socket";
        perror(err_msg);
        return;
    }

    if (listen(sk, BACKLOG) < 0) {
        err_msg = "can't listen";
        perror(err_msg);
        return;
    }
}


stream_socket *tcp_server_socket::accept_one_client() {
    if (err_msg) {
        perror(err_msg);
        throw std::runtime_error(err_msg);
    }

    socklen_t addrLen = sizeof(ipv4addr);
    int client_sk = accept(sk, (sockaddr *) &ipv4addr, &addrLen);

    if (client_sk < 0) {
        err_msg = "can't accept";
        perror(err_msg);
        throw std::runtime_error(err_msg);
    }

    tcp_connection_socket *retSocket = new tcp_connection_socket(client_sk);
    acceptedSockets.push_back(retSocket);

    return (stream_socket *) retSocket;
}

void tcp_server_socket::close() {
    if (sk >= 0) {
        ::shutdown(sk, SHUT_RDWR);
        ::close(sk);
    }

    for (auto i = acceptedSockets.begin(); i != acceptedSockets.end(); ++i)
        (*i)->close();

    std::cerr << "server socket is closed\n";
}

tcp_server_socket::~tcp_server_socket() {
    if (sk >= 0)
        close();
}
