#pragma once

#include <thread>
#include "protocol.h"
#include "tcp_wrapper.h"

static const hostname HOSTNAME = "localhost";
static const tcp_port HOSTPORT = 8000;


class DataStorage
{

};

class TradeConnection
{
	stream_socket* sk;
	std::thread handlerThread;

	void handle(DataStorage *dataStorage);

public:
	TradeConnection(const stream_socket *sk, DataStorage* dataStorage)
	{
		handlerThread = std::thread(handle, dataStorage);
	}

	void close();
};


class TradeServer
{
	tcp_server_socket *serverSocket = nullptr;
	std::thread listenerThread;
	std::list<TradeConnection> connections;
	bool stopped = false;
	DataStorage dataStorage;

	void listenConnection();

public:
	TradeServer()
	{
		serverSocket = new tcp_server_socket(HOSTNAME, HOSTPORT);
	}

	void start()
	{
		listenerThread = std::thread(listenConnection);
	}

	void stop();
};