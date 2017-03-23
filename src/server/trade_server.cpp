#include <algorithm>
#include <iostream>
#include "trade_server.h"


static void newLotRequestHandler(stream_socket *sk, Packet *packet, TradeConnection::Context *context) {
    std::cerr << context->getUid() << ":" << "new lot request handler\n";

    NewLotRequest *request = (NewLotRequest *) packet->getBody();
    uint32_t lotId = context->getDataStorage()->addNewLot(request->getStartPrice(), context->getUid(),
                                                          request->getDescription());
    Packet::constructNewLotResponse(lotId).writeToStreamSocket(sk);
}


static void listLotsRequestHandler(stream_socket *sk, Packet *packet, TradeConnection::Context *context) {
    std::cerr << context->getUid() << ":" << "list lots request handler\n";

    Packet::constructListLotsResponse(context->getDataStorage()->getShortInfoList()).writeToStreamSocket(sk);
}


static void lotDetailsRequestHandler(stream_socket *sk, Packet *packet, TradeConnection::Context *context) {
    std::cerr << context->getUid() << ":" << "lot details request handler\n";

    LotDetailsRequest *request = (LotDetailsRequest *) packet->getBody();
    uint32_t lotId = request->getLotId();
    LotFullInfo lotFullInfo = context->getDataStorage()->getLotInfoById(lotId);
    Packet::constructLotDetailsResponse(lotFullInfo).writeToStreamSocket(sk);
}


static void makeBetRequestHandler(stream_socket *sk, Packet *packet, TradeConnection::Context *context) {
    std::cerr << context->getUid() << ":" << "make bet request handler\n";

    MakeBetRequest *request = (MakeBetRequest *) packet->getBody();
    context->getDataStorage()->makeBet(request->getBet());
    Packet::constructStatus(true).writeToStreamSocket(sk);
}


static void closeLotRequestHandler(stream_socket *sk, Packet *packet, TradeConnection::Context *context) {
    std::cerr << context->getUid() << ":" << "close lot request handler\n";

    CloseLotRequest *request = (CloseLotRequest *) packet->getBody();
    context->getDataStorage()->closeLot(request->getLotId());
    Packet::constructStatus(true).writeToStreamSocket(sk);
}


static std::map<Body::BodyType, void (*)(stream_socket *, Packet *, TradeConnection::Context *)> messagesHandlers = {
        {Body::BodyType::NEW_LOT_REQ,   newLotRequestHandler},
        {Body::BodyType::LIST_LOTS_REQ, listLotsRequestHandler},
        {Body::BodyType::LOT_DET_REQ,   lotDetailsRequestHandler},
        {Body::BodyType::MAKE_BET_REQ,  makeBetRequestHandler},
        {Body::BodyType::CLOSE_LOT_REQ, closeLotRequestHandler},
};


void TradeConnection::handle() {
    Packet::constructAuthorisationResponse(context->getUid()).writeToStreamSocket(sk);

    try {
        Packet packet;
        while (true) {
            packet.readFromStreamSocket(sk);
            if (packet.getBody()->getType() == Body::BodyType::BYE)
                break;
            messagesHandlers[packet.getBody()->getType()](sk, &packet, context);
        }
    } catch (std::exception e) {
        std::cerr << e.what() << '\n';
    }
}


/*
 * TradeServer implementation:
 */

void TradeServer::listenConnection() {
    try {
        std::cerr << "start listen connections\n";

        while (true) {
            stream_socket *streamSocket = serverSocket->accept_one_client();
            connections.push_back(new TradeConnection(streamSocket, &dataStorage));
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
    }
}


void TradeServer::start() {
    std::cerr << "trade server starts\n";
    listenerThread = std::thread(listenConnectionWrapper, this);
}


TradeServer::~TradeServer() {
    std::cerr << "server closes\n";
    serverSocket->close();
    listenerThread.join();
    for (auto i = connections.begin(); i != connections.end(); ++i) {
        delete *i;
    }
}


uint32_t DataStorage::addNewUser() {
    std::unique_lock<std::mutex> lock(mtx);

    uint32_t uid;
    connectedUsersIds.emplace(uid = freeUid++);

    return uid;
}


LotFullInfo DataStorage::getLotInfoById(uint32_t lotId) {
    std::unique_lock<std::mutex> lock(mtx);
    return lotsData[lotId];
}


uint32_t DataStorage::addNewLot(uint32_t startPrice, uint32_t ownerId, std::string description) {
    std::unique_lock<std::mutex> lock(mtx);

    uint32_t newLotId = lotsData.size() + 1;
    lotsData[newLotId] = LotFullInfo(newLotId, ownerId, true, description, startPrice, std::list<Bet>());

    return newLotId;
}


std::list<LotShortInfo> DataStorage::getShortInfoList() {
    std::unique_lock<std::mutex> lock(mtx);
    std::list<LotShortInfo> shortInfoList;

    for (auto i = lotsData.begin(); i != lotsData.end(); ++i) {
        LotFullInfo &lotInfo = i->second;
        shortInfoList.push_back(LotShortInfo(lotInfo.lotId, lotInfo.opened, lotInfo.startPrice, lotInfo.getBestPrice(),
                                             lotInfo.description));
    }

    return shortInfoList;
}


void DataStorage::makeBet(Bet bet) {
    std::unique_lock<std::mutex> lock(mtx);
    uint32_t lotId = bet.productId;

    if (lotsData.at(lotId).opened)
        lotsData[lotId].bets.push_back(bet);
}


void DataStorage::closeLot(int lotId) {
    std::unique_lock<std::mutex> lock(mtx);
    lotsData.at(lotId).opened = false;
}
