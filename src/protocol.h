#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>
#include <typeindex>
#include <stdint.h>

#include "stream_socket.h"

class Serializable
{
public:
	virtual void writeToStreamSocket(stream_socket &sk) = 0;
	virtual void readFromStreamSocket(stream_socket &sk) = 0;
};


class Body : Serializable
{
public:
	virtual BodyType getType() = 0;

	enum BodyType
	{
		AUTH_RESP,
		NEW_LOT_REQ,
		NEW_LOT_RESP,
		LIST_LOTS_REQ,
		LIST_LOTS_RESP,
		MAKE_BET_REQ,
		MAKE_BET_RESP,
		LOT_DET_REQ,
		LOT_DET_RESP,
		CLOSE_LOT_REQ,
		CLOSE_LOT_RESP
	};
};


class Packet : Serializable
{
	Body *body = nullptr;

public:
	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;
};


class AuthorisationResponse : Body
{
	uint32_t customerId;

public:
	BodyType getType() override {
		return AUTH_RESP;
	};

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new AuthorisationResponse();
	}
};


class NewLotRequest : Body
{
	std::string description;
	uint32_t startPrice;

public:
	BodyType getType() override {
		return NEW_LOT_REQ;
	};

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new NewLotRequest();
	}
};


class NewLotResponse : Body
{
	bool accepted;

public:
	BodyType getType() override {
		return NEW_LOT_RESP;
	};

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new NewLotResponse();
	}
};


struct LotShortInfo
{
	uint32_t lotId;
	bool opened;
	std::string description;
	uint32_t startPrice;
	uint32_t bestPrice;

	LotShortInfo(uint32_t lotId, bool opened, uint32_t startPrice, uint32_t bestPrice, std::string description)
	{
		this->lotId = lotId;
		this->opened = opened;
		this->startPrice = startPrice;
		this->bestPrice = bestPrice;
		this->description = description;
	}
};


class ListLotsRequest : Body
{
public:
	BodyType getType() override {
		return LIST_LOTS_REQ;
	}

	void writeToStreamSocket(stream_socket &sk) override {}
	void readFromStreamSocket(stream_socket &sk) override {}

	static Serializable* generator() {
		return (Serializable *) new ListLotsRequest();
	}
};


class ListLotsResponse : Body
{
	std::list<LotShortInfo> lotsInfo;

public:
	BodyType getType() {
		return BodyType::LIST_LOTS_RESP;
	}

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new ListLotsResponse();
	}
};


struct Bet
{
	uint32_t productId;
	uint32_t customerId;
	uint32_t newPrice;

	Bet(uint32_t productId, uint32_t customerId, uint32_t newPrice)
	{
		this->productId = productId;
		this->customerId = customerId;
		this->newPrice = newPrice;
	}
};


class MakeBetRequest : Body
{
	Bet bet;

public:
	BodyType getType() {
		return MAKE_BET_REQ;
	}

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new MakeBetRequest();
	}
};


class MakeBetResponse : Body
{
	bool accepted;

public:
	BodyType getType() {
		return MAKE_BET_RESP;
	}

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new MakeBetResponse();
	}
};


struct LotFullInfo
{
	uint32_t lotId;
	uint32_t ownerId;
	bool opened;
	std::string description;
	uint32_t startPrice;
	std::list<Bet> bets;

	LotFullInfo(uint32_t lotId, uint32_t ownerId, bool opened, std::string description, uint32_t startPrice, std::list<Bet> &bets)
	{
		this->lotId = lotId;
		this->ownerId = ownerId;
		this->opened = opened;
		this->description = description;
		this->startPrice = startPrice;
		this->bets = bets;
	}
};


class LotDetailsRequest : Body
{
public:
	BodyType getType() {
		return LOT_DET_REQ;
	}

	void writeToStreamSocket(stream_socket &sk) override {}
	void readFromStreamSocket(stream_socket &sk) override {}

	static Serializable* generator() {
		return (Serializable *) new LotDetailsRequest();
	}
};


class LotDetailsResponse : Body
{
	LotFullInfo lotDetails;

public:
	BodyType getType() {
		return LOT_DET_RESP;
	}

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new LotDetailsResponse();
	}
};


class CloseLotRequest : Body
{
	uint32_t lotId;

public:
	BodyType getType() {
		return CLOSE_LOT_REQ;
	}

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new CloseLotRequest();
	}
};


class CloseLotResponse : Body
{
	bool closed;

public:
	BodyType getType() {
		return CLOSE_LOT_RESP;
	}

	void writeToStreamSocket(stream_socket &sk) override;
	void readFromStreamSocket(stream_socket &sk) override;

	static Serializable* generator() {
		return (Serializable *) new CloseLotResponse();
	}
};
