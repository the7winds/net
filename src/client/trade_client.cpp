#include "trade_client.h"

void TradeClient::closeLot(uint32_t lotId) {
    Packet::constructCloseLotRequest(lotId).writeToStreamSocket(sk);
    received.readFromStreamSocket(sk);

    if (received.getBody()->getType() == Body::BodyType::BYE)
        throw std::exception();

    Status *status = (Status *) received.getBody();
    std::cout << (status->getStatus() ? "closed" : "try again") << '\n';
}

void TradeClient::makeBet(uint32_t lotId, uint32_t newPrice) {
    Packet::constructMakeBetRequest(lotId, newPrice).writeToStreamSocket(sk);
    received.readFromStreamSocket(sk);

    if (received.getBody()->getType() == Body::BodyType::BYE)
        throw std::runtime_error("server closed");

    Status *status = (Status *) received.getBody();
    std::cout << (status->getStatus() ? "your bet is accepted" : "try again") << '\n';
}

void TradeClient::lotDetails(uint32_t lotId) {
    Packet::constructLotDetailsRequest(lotId).writeToStreamSocket(sk);
    received.readFromStreamSocket(sk);
    if (received.getBody()->getType() == Body::BodyType::BYE)
        throw std::exception();

    LotDetailsResponse *lotDetailsResponse = (LotDetailsResponse *) received.getBody();
    const LotFullInfo &lotFullInfo = lotDetailsResponse->getLotDetails();

    std::cout << "lot id: " << lotFullInfo.lotId << '\n';
    std::cout << "lot owner id: " << lotFullInfo.ownerId << '\n';
    std::cout << "lot status: " << (lotFullInfo.opened ? "opened" : "closed") << '\n';
    std::cout << "start price: " << lotFullInfo.startPrice << '\n';
    std::cout << "description: " << lotFullInfo.description << '\n';
    std::cout << "best price: " << lotFullInfo.getBestPrice() << '\n';

    std::cout << "bets:\n";
    std::cout << "customer id | new price\n";
    for (auto b : lotFullInfo.bets)
        std::cout << b.customerId << " : " << b.newPrice << '\n';
}

void TradeClient::listLots() {
    Packet::constructListLotsRequest().writeToStreamSocket(sk);
    received.readFromStreamSocket(sk);
    if (received.getBody()->getType() == Body::BodyType::BYE)
        throw std::exception();

    ListLotsResponse *listLotsResponse = (ListLotsResponse *) received.getBody();

    std::cout << "lots info:\n";
    for (auto &a : listLotsResponse->getLotsInfo()) {
        std::cout << "lot id: " << a.lotId << '\n';
        std::cout << "status: " << (a.opened ? "open" : "closed") << '\n';
        std::cout << "start price:" << a.startPrice << '\n';
        std::cout << "best price:" << a.bestPrice << '\n';
        std::cout << std::endl;
    }
}

void TradeClient::newLot(std::string &description, uint32_t startPrice) {
    Packet::constructNewLotRequest(description, startPrice).writeToStreamSocket(sk);
    received.readFromStreamSocket(sk);
    std::cout << "lot id: " << ((NewLotResponse *) received.getBody())->getLotId() << '\n';
}

void TradeClient::start() {
    sk->connect();

    received.readFromStreamSocket(sk);
    if (received.getBody()->getType() == Body::BodyType::BYE)
        throw std::exception();
    AuthorisationResponse* authorisationResponse = (AuthorisationResponse *) received.getBody();

    std::cout << "Connection success! Your id: " << authorisationResponse->getId() << '\n';
}

TradeClient::~TradeClient() {
    if (sk) {
        delete sk;
    }
}
