#pragma once

#include <thread>
#include <mutex>
#include <set>
#include "../protocol.h"
#include "../tcp_socket.h"
#include <iostream>
#include "../au_stream_socket.h"

#define DEFAULT_ADDR "127.0.0.1"
#define DEFAULT_PORT 40001


class DataStorage {
    uint32_t freeUid = 0;
    std::mutex mtx;
    std::set<uint32_t> connectedUsersIds;
    std::map<uint32_t, LotFullInfo> lotsData;

public:
    uint32_t addNewUser();

    LotFullInfo getLotInfoById(uint32_t lotId);

    uint32_t addNewLot(uint32_t startPrice, uint32_t ownerId, std::string description);

    std::list<LotShortInfo> getShortInfoList();

    bool makeBet(uint32_t uid, const Bet& bet);

    bool closeLot(int uid, int lotId);
};

class TradeConnection {
    std::thread handlerThread;
    stream_socket *sk;

    void handle();

    static void handleWrapper(TradeConnection *self) {
        self->handle();
    }

public:
    TradeConnection(stream_socket *sk, DataStorage *dataStorage) : sk(sk) {
        context = new Context(dataStorage->addNewUser(), dataStorage);
        handlerThread = std::thread(handleWrapper, this);
        std::cerr << "created new connection CONNECTION_ID:" << context->getUid() << "\n";
    }

    ~TradeConnection() {
        handlerThread.join();
        delete context;
    }

    class Context {
        uint32_t uid;
        DataStorage *dataStorage;

    public:
        Context(uint32_t uid, DataStorage *dataStorage) : uid(uid), dataStorage(dataStorage) {}

        uint32_t getUid() {
            return uid;
        }

        DataStorage *getDataStorage() {
            return dataStorage;
        }
    };

private:
    Context *context;
};


class TradeServer {
    stream_server_socket *serverSocket = nullptr;
    std::thread listenerThread;
    std::list<TradeConnection*> connections;
    DataStorage dataStorage;
    std::atomic_bool closed;

    void listenConnection();

    static void listenConnectionWrapper(TradeServer *self) {
        self->listenConnection();
    }

public:
    TradeServer(const char* ip, tcp_port port) {
        char *socket_type = getenv("STREAM_SOCK_TYPE");
        if (socket_type == NULL || std::string(socket_type) == "AU") {
            serverSocket = new au_stream_server_socket(ip, port);
        } else {
            serverSocket = new tcp_server_socket(ip, port);
        }
        closed.store(false);
    }

    void start();

    ~TradeServer();
};
