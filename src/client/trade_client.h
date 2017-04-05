#pragma once

#include "../tcp_socket.h"
#include "../server/trade_server.h"

class TradeClient {
    uint32_t uid;
    Packet received;
    tcp_client_socket *sk = nullptr;

public:
    TradeClient(const char *serverAddr, tcp_port port = DEFAULT_PORT) {
        sk = new tcp_client_socket(serverAddr, port);
    }

    void start();

    void newLot(std::string &description, uint32_t startPrice);

    void listLots();

    void lotDetails(uint32_t lotId);

    void makeBet(uint32_t lotId, uint32_t newPrice);

    void closeLot(uint32_t lotId);

    ~TradeClient();

    void bye();
};
