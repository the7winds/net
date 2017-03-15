#include "trade_server.h"
#include <iostream>


int main()
{
	TradeServer *tradeServer = new TradeServer();
	tradeServer->start();

	std::string input;
	while (true) {
		std::getline(std::cin, input);
		if (input == "q") {
			break;
		}
	}

	tradeServer->stop();
	delete tradeServer;

	return 0;
}