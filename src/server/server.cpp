#include "trade_server.h"

static const char *QUIT = "q";

int main() {
    TradeServer tradeServer;
    tradeServer.start();

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input == QUIT)
            break;
    }

    return 0;
}