#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>
#include <typeindex>
#include <stdint.h>
#include <algorithm>

#include "stream_socket.h"
#include "util.h"


struct LotShortInfo {
    uint32_t lotId;
    bool opened;
    std::string description;
    uint32_t startPrice;
    uint32_t bestPrice;


    LotShortInfo(uint32_t lotId, bool opened, uint32_t startPrice, uint32_t bestPrice, std::string description) {
        this->lotId = lotId;
        this->opened = opened;
        this->startPrice = startPrice;
        this->bestPrice = bestPrice;
        this->description = description;
    }
};


struct Bet {
    uint32_t productId;
    uint32_t customerId;
    uint32_t newPrice;

    Bet() {}

    Bet(uint32_t productId, uint32_t customerId, uint32_t newPrice) {
        this->productId = productId;
        this->customerId = customerId;
        this->newPrice = newPrice;
    }
};


struct LotFullInfo {
    uint32_t lotId;
    uint32_t ownerId;
    bool opened;
    std::string description;
    uint32_t startPrice;
    std::list<Bet> bets;


    LotFullInfo() {}


    LotFullInfo(uint32_t lotId, uint32_t ownerId, bool opened, std::string description, uint32_t startPrice,
                std::list<Bet> bets) {
        this->lotId = lotId;
        this->ownerId = ownerId;
        this->opened = opened;
        this->description = description;
        this->startPrice = startPrice;
        this->bets = bets;
    }


    uint32_t getBestPrice() const {
        return std::max_element(bets.begin(), bets.end(),
                                [](const Bet &a, const Bet &b) { return a.newPrice < b.newPrice; })->newPrice;
    }
};


class Serializable {
public:
    virtual void writeToStreamSocket(stream_socket *sk) = 0;

    virtual void readFromStreamSocket(stream_socket *sk) = 0;
};


class Body : public Serializable {
public:

    enum BodyType {
        AUTH_RESP,
        NEW_LOT_REQ,
        NEW_LOT_RESP,
        LIST_LOTS_REQ,
        LIST_LOTS_RESP,
        MAKE_BET_REQ,
        LOT_DET_REQ,
        LOT_DET_RESP,
        CLOSE_LOT_REQ,
        STATUS,
        BYE
    };

    virtual BodyType getType() = 0;

    virtual ~Body() {};
};


class Packet : public Serializable {
    Body *body = nullptr;

public:
    Packet() {}

    Packet(Body *body) {
        this->body = body;
    }

    Body *getBody() {
        return body;
    }

    ~Packet() {
        delete body;
    }


    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;

    static Packet constructAuthorisationResponse(uint32_t customerId);

    static Packet constructNewLotResponse(uint32_t lotId);

    static Packet constructListLotsResponse(std::list<LotShortInfo> lotsShortInfoList);

    static Packet constructLotDetailsResponse(LotFullInfo &info);

    static Packet constructStatus(bool status);

    static Packet constructNewLotRequest(std::string description, uint32_t startPrice);

    static Packet constructListLotsRequest();

    static Packet constructLotDetailsRequest(uint32_t lotId);

    static Packet constructMakeBetRequest(uint32_t lotId, uint32_t newPrice);

    static Packet constructCloseLotRequest(uint32_t lotId);

    static Packet constructBye();
};


class AuthorisationResponse : public Body {
    uint32_t customerId;

public:
    AuthorisationResponse(uint32_t customerId = 0) : customerId(customerId) {}

    BodyType getType() override { return AUTH_RESP; };

    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;

    static Serializable *generator() { return (Serializable *) new AuthorisationResponse(); }

    uint32_t getId() { return customerId; }

    ~AuthorisationResponse() override {}
};


class NewLotRequest : public Body {
    std::string description;
    uint32_t startPrice;

public:
    NewLotRequest() {}

    NewLotRequest(std::string description, uint32_t startPrice) : description(description), startPrice(startPrice) {};

    BodyType getType() override { return NEW_LOT_REQ; };

    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;

    static Serializable *generator() { return (Serializable *) new NewLotRequest(); }

    std::string getDescription() { return description; }

    uint32_t getStartPrice() { return startPrice; }
};


class NewLotResponse : public Body {
    uint32_t lotId;

public:
    NewLotResponse(uint32_t lotId = 0) {
        this->lotId = lotId;
    }


    BodyType getType() override {
        return NEW_LOT_RESP;
    };

    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;

    uint32_t getLotId() {
        return lotId;
    }

    static Serializable *generator() {
        return (Serializable *) new NewLotResponse();
    }
};


class ListLotsRequest : public Body {
public:
    BodyType getType() override {
        return LIST_LOTS_REQ;
    }


    void writeToStreamSocket(stream_socket *sk) override {}


    void readFromStreamSocket(stream_socket *sk) override {}


    static Serializable *generator() {
        return (Serializable *) new ListLotsRequest();
    }
};


class ListLotsResponse : public Body {
    std::list<LotShortInfo> lotsInfo;

public:
    ListLotsResponse() {}

    ListLotsResponse(std::list<LotShortInfo> lotsInfo) {
        this->lotsInfo = lotsInfo;
    }


    BodyType getType() {
        return BodyType::LIST_LOTS_RESP;
    }


    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;


    static Serializable *generator() {
        return (Serializable *) new ListLotsResponse();
    }

    const std::list<LotShortInfo>& getLotsInfo() {
        return lotsInfo;
    }
};


class MakeBetRequest : public Body {
    Bet bet;

public:
    MakeBetRequest() {}

    MakeBetRequest(uint32_t lotId, uint32_t newPrice) {
        bet.productId = lotId;
        bet.newPrice = newPrice;
    }

    BodyType getType() {
        return MAKE_BET_REQ;
    }


    Bet getBet() {
        return bet;
    }


    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;


    static Serializable *generator() {
        return (Serializable *) new MakeBetRequest();
    }
};


class LotDetailsRequest : Body {
    uint32_t lotId;

public:
    LotDetailsRequest() {}

    LotDetailsRequest(uint32_t lotId) {
        this->lotId = lotId;
    }

    BodyType getType() {
        return LOT_DET_REQ;
    }


    uint32_t getLotId() {
        return lotId;
    }


    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;


    static Serializable *generator() {
        return (Serializable *) new LotDetailsRequest();
    }

    ~LotDetailsRequest() {}
};


class LotDetailsResponse : Body {
    LotFullInfo lotDetails;

public:
    LotDetailsResponse() {}

    LotDetailsResponse(LotFullInfo &lotFullInfo) {
        lotDetails = lotFullInfo;
    }

    BodyType getType() {
        return LOT_DET_RESP;
    }

    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;

    static Serializable *generator() {
        return (Serializable *) new LotDetailsResponse();
    }

    const LotFullInfo &getLotDetails() {
        return lotDetails;
    }
};


class CloseLotRequest : Body {
    uint32_t lotId = 0;

public:
    CloseLotRequest() {}

    CloseLotRequest(uint32_t lotId) {
        this->lotId = lotId;
    }

    BodyType getType() {
        return CLOSE_LOT_REQ;
    }


    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;


    static Serializable *generator() {
        return (Serializable *) new CloseLotRequest();
    }


    uint32_t getLotId();

    ~CloseLotRequest() {}
};


class Status : Body {
    bool status;

public:
    Status() {}

    Status(bool status) {
        this->status = status;
    }

    BodyType getType() {
        return STATUS;
    }

    void writeToStreamSocket(stream_socket *sk) override;

    void readFromStreamSocket(stream_socket *sk) override;

    static Serializable *generator() {
        return new Status();
    }

    ~Status() {}

    bool getStatus();
};


class Bye : Body {
public:
    BodyType getType() {
        return BYE;
    }

    void writeToStreamSocket(stream_socket *sk) override {}

    void readFromStreamSocket(stream_socket *sk) override {}

    static Serializable *generator() {
        return new Bye();
    }
};