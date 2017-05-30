#include <algorithm>
#include <iostream>
#include <signal.h>
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
    try {
        LotFullInfo lotFullInfo = context->getDataStorage()->getLotInfoById(lotId);
        Packet::constructLotDetailsResponse(lotFullInfo).writeToStreamSocket(sk);
    } catch (...) {
        Packet::constructStatus(false).writeToStreamSocket(sk);
    }
}


static void makeBetRequestHandler(stream_socket *sk, Packet *packet, TradeConnection::Context *context) {
    std::cerr << context->getUid() << ":" << "make bet request handler\n";

    MakeBetRequest *request = (MakeBetRequest *) packet->getBody();
    bool status = context->getDataStorage()->makeBet(context->getUid(), request->getBet());
    Packet::constructStatus(status).writeToStreamSocket(sk);
}


static void closeLotRequestHandler(stream_socket *sk, Packet *packet, TradeConnection::Context *context) {
    std::cerr << context->getUid() << ":" << "close lot request handler\n";

    CloseLotRequest *request = (CloseLotRequest *) packet->getBody();
    bool status = context->getDataStorage()->closeLot(context->getUid(), request->getLotId());
    Packet::constructStatus(status).writeToStreamSocket(sk);
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
    } catch (std::exception &e) {
        /*
         * здесь мы окажемся, только если что-то пойдёт не так во время чтения,
         * например клиент отвалился и не смог нас оповестить
         * так, мы просто закончим соединение
         */
        std::cerr << context->getUid() << ":" << e.what() << '\n';
    }
    std::cerr << context->getUid() << ":" << "connection closed\n";
}


/*
 * TradeServer implementation:
 */

void TradeServer::listenConnection() {
    try {
        std::cerr << "start listen connections\n";

        while (!closed.load()) {
            stream_socket *streamSocket = serverSocket->accept_one_client();
            connections.push_back(new TradeConnection(streamSocket, &dataStorage));
        }
    } catch (std::exception &e) {
        /*
         * здесь мы окажемся, если не можем принять новое соединение
         * или когда сервер будет завершатся, мы закроем сокет и тоже окажемся здесь
         */
        std::cerr << e.what() << '\n';
    }
}


void TradeServer::start() {
    std::cerr << "trade server starts\n";
    listenerThread = std::thread(listenConnectionWrapper, this);
}


TradeServer::~TradeServer() {
    std::cerr << "server closes\n";
    closed.store(true);
    kill(0, SIGUSR1);
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
    return lotsData.at(lotId);
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


bool DataStorage::makeBet(uint32_t uid, const Bet &bet) try {
    std::unique_lock<std::mutex> lock(mtx);
    uint32_t lotId = bet.productId;

    if (lotsData.at(lotId).ownerId != uid
        && lotsData.at(lotId).opened
        && lotsData.at(lotId).startPrice <= bet.newPrice) {
        lotsData[lotId].bets.push_back(bet);
        return true;
    }

    return false;
} catch (...) {
    /*
     * здесь окажемся если id был некорректным,
     * тогда мы не можем сделать ставку
     */
    return false;
}


bool DataStorage::closeLot(int uid, int lotId) try {
    std::unique_lock<std::mutex> lock(mtx);

    if (lotsData.at(lotId).ownerId == uid) {
        lotsData[lotId].opened = false;
        return true;
    }

    return false;
} catch (...) {
    /*
     * здесь окажемся если id был некорректным,
     * тогда мы не можем закрыть лот
     */
    return false;
}
