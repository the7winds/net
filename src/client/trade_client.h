#pragma once

#include "../tcp_socket.h"
#include "../server/trade_server.h"
#include "../au_stream_socket.h"

class TradeClient {
    uint32_t uid;
    Packet received;
    stream_client_socket *sk = nullptr;

public:
    TradeClient(const char *serverAddr, tcp_port my_port, tcp_port server_port = DEFAULT_PORT) {
        char *socket_type = getenv("STREAM_SOCK_TYPE");
        if (socket_type == NULL || std::string(socket_type) == "AU") {
            sk = new au_stream_client_socket(serverAddr, my_port, server_port);
        } else {
            sk = new tcp_client_socket(serverAddr, server_port);
        }
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
