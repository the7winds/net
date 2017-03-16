#include <iostream>
#include "trade_client.h"

static const char *NEW_LOT = "nl";
static const char *LIST_LOTS = "ll";
static const char *LOT_DETAILS = "ld";
static const char *MAKE_BET = "b";
static const char *CLOSE_LOT = "c";
static const char *QUIT = "q";

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
            } else if (input == QUIT) {
                break;
            } else {
                std::cerr << "unsupported operation\n";
            }
        }
    } catch (std::exception e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}