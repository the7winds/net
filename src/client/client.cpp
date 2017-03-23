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
        "h - show this message";

int main(int argc, char **argv) {
    uint32_t lotId;
    uint32_t startPrice;
    uint32_t newPrice;
    std::string description;
    TradeClient tradeClient(argv[1]);

    tradeClient.start();

    try {
        while (true) {
            std::string input;
            std::getline(std::cin, input);

            if (input == NEW_LOT) {
                std::cin >> description >> startPrice;
                tradeClient.newLot(description, startPrice);
            } else if (input == LIST_LOTS) {
                tradeClient.listLots();
            } else if (input == LOT_DETAILS) {
                std::cin >> lotId;
                tradeClient.lotDetails(lotId);
            } else if (input == MAKE_BET) {
                std::cin >> lotId >> newPrice;
                tradeClient.makeBet(lotId, newPrice);
            } else if (input == CLOSE_LOT) {
                std::cin >> lotId;
                tradeClient.closeLot(lotId);
            } else if (input == HELP) {
                std::cerr << HELP_MSG;
            } else if (input == QUIT) {
                break;
            } else {
                std::cerr << "unsupported operation\n";
                std::cerr << HELP_MSG;
            }
        }
    } catch (std::exception e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}