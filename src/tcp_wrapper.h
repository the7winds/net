#pragma once

#include "stream_socket.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <exception>
#include <mutex>
#include <netinet/in.h>
#include <list>


class tcp_connection_socket : stream_socket
{
	int sk;
	const char *err_msg = nullptr;
	std::mutex mtx;
	bool closed = false;

	tcp_connection_socket(int sk);

	friend stream_socket *tcp_server_socket::accept_one_client();

public:
	void send(const void *buf, size_t size) override;

	void recv(void *buf, size_t size) override;

	void close();

	~tcp_connection_socket();
};


class tcp_client_socket : stream_client_socket
{
	int sk;
	const char *err_msg = nullptr;
	std::mutex mtx;
	sockaddr_in ipv4addr;

public:
	tcp_client_socket(const char *addr, uint16_t port);

	void send(const void *buf, size_t size) override;

	void recv(void *buf, size_t size) override;

	void connect() override;

	~tcp_client_socket() override;
};


class tcp_server_socket : stream_server_socket
{
	const static int BACKLOG = 5;
	int sk;
	const char *err_msg = nullptr;
	std::mutex mtx;
	sockaddr_in ipv4addr;
	std::list<tcp_connection_socket *> acceptedSockets;
	bool closed = false;

public:
	tcp_server_socket(const char *addr, uint16_t port);

	stream_socket *accept_one_client() override;

	void close();

	~tcp_server_socket() override;
};
