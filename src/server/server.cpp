#include "trade_server.h"

static const char *QUIT = "q";

int main(int argc, char** argv) {
    const char* ip = DEFAULT_ADDR;
    tcp_port port = DEFAULT_PORT;

    if (argc > 2)
        port = atoi(argv[2]);
    if (argc > 1)
        ip = argv[1];

    fix_au_port(port);

    TradeServer tradeServer(ip, port);
    tradeServer.start();

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input == QUIT)
            break;
    }

    free_au_port(port);

    return 0;
}
