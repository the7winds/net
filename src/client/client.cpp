#include <iostream>
#include "trade_client.h"

static const std::string NEW_LOT = "nl";
static const std::string LIST_LOTS = "ll";
static const std::string LOT_DETAILS = "ld";
static const std::string MAKE_BET = "b";
static const std::string CLOSE_LOT = "c";
static const std::string QUIT = "q";
static const std::string HELP = "h";

static const std::string HELP_MSG =
        "help:\n"
        "nl <description> <price> - new lot\n"
        "ll - list lots\n"
        "ld <lot id> - lot details\n"
        "b <lot id> <new price> - make bet\n"
        "c <lot id> - close lot\n"
        "q - quit\n"
        "h - show this message\n";

int main(int argc, char **argv) {
    uint32_t lotId;
    uint32_t startPrice;
    uint32_t newPrice;
    std::string description;
    const char* ip = DEFAULT_ADDR;
    tcp_port server_port = DEFAULT_PORT;
    tcp_port my_port = 1;

    if (argc > 3)
        server_port = atoi(argv[3]);
    if (argc > 2)
        my_port = atoi(argv[2]);
    if (argc > 1)
        ip = argv[1];

    try {
        fix_au_port(my_port);

        TradeClient tradeClient(ip, my_port, server_port);
        tradeClient.start();

        while (true) {
            std::string cmd;
            std::string w1, w2;
            std::cin >> cmd;

            if (cmd == NEW_LOT) {
                std::cin >> w1 >> w2;
                description = w1;
                startPrice = atoi(w2.c_str());
                tradeClient.newLot(description, startPrice);
            } else if (cmd == LIST_LOTS) {
                tradeClient.listLots();
            } else if (cmd == LOT_DETAILS) {
                std::cin >> w1;
                lotId = atoi(w1.c_str());
                tradeClient.lotDetails(lotId);
            } else if (cmd == MAKE_BET) {
                std::cin >> w1 >> w2;
                lotId = atoi(w1.c_str());
                newPrice = atoi(w2.c_str());
                tradeClient.makeBet(lotId, newPrice);
            } else if (cmd == CLOSE_LOT) {
                std::cin >> w1;
                lotId = atoi(w1.c_str());
                tradeClient.closeLot(lotId);
            } else if (cmd == HELP) {
                std::cerr << HELP_MSG;
            } else if (cmd == QUIT) {
                tradeClient.bye();
                break;
            } else {
                std::cerr << "unsupported operation\n";
                std::cerr << HELP_MSG;
            }
        }

	free_au_port(my_port);
    } catch (std::exception &e) {
        /*
         * здесь мы окажемся если обработка какого-нибудь запроса прошла неверно
         * или если разрешение имени сервера произошло неуспешно
         */
        std::cerr << e.what() << '\n';
    }

    return 0;
}
