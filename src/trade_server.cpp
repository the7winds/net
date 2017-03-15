#include "trade_server.h"


static void newLotRequestHandler(stream_socket *sk, Packet *packet, DataStorage *dataStorage)
{

}


static void listLotsRequestHandler(stream_socket *sk, Packet *packet, DataStorage *dataStorage)
{

}


static void lotDetailsRequestHandler(stream_socket *sk, Packet *packet, DataStorage *dataStorage)
{

}


static void makeBetRequestHandler(stream_socket *sk, Packet *packet, DataStorage *dataStorage)
{

}


static void closeLotRequestHandler(stream_socket *sk, Packet *packet, DataStorage *dataStorage)
{

}


static std::map<Body::BodyType, void (*)(stream_socket *, Packet *, DataStorage *)> messagesHandlers = {
		{Body::BodyType::NEW_LOT_REQ,   newLotRequestHandler},
		{Body::BodyType::LIST_LOTS_REQ, listLotsRequestHandler},
		{Body::BodyType::LOT_DET_REQ,   lotDetailsRequestHandler},
		{Body::BodyType::MAKE_BET_REQ,  makeBetRequestHandler},
		{Body::BodyType::CLOSE_LOT_REQ, closeLotRequestHandler}
};


void TradeConnection::handle(DataStorage *dataStorage)
{
	while (true) {
		Packet packet;
		packet.readFromStreamSocket(sk);
		messagesHandlers[packet.getBody()->getType()](sk, &packet, dataStorage);
	}
}


void TradeConnection::close()
{
	handlerThread.join();
}


void TradeServer::listenConnection()
{
	while (true) {
		stream_socket *streamSocket = serverSocket->accept_one_client();
		connections.push_back(TradeConnection(streamSocket, &dataStorage));
	}
}


void TradeServer::stop()
{
	serverSocket->close();
	listenerThread.join();
	for (auto i = connections.begin(); i != connections.end(); ++i) {
		i->close();
	}

	stopped = true;
}


void ~TradeServer::TradeServer()
{
	if (!stopped) {
		stop();
	}
}