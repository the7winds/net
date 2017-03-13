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
};


class AuthorisationResponse : Body
{
	int32_t customerId;

public:
	static Serializable* generator() {
		return (Serializable *) new AuthorisationResponse();
	}
};


class NewLotRequest : Body
{
	std::string description;
	int startPrice;

public:
	static Serializable* generator() {
		return (Serializable *) new NewLotRequest();
	}
};


class NewLotResponse : Body
{
	bool accepted;

public:
	static Serializable* generator() {
		return (Serializable *) new NewLotResponse();
	}
};


class LotShortInfo
{
	int lotId;
	bool opened;
	std::string *description;
	int startPrice;
	int bestPrice;
};


class ListLotsRequest : Body
{
public:
	static Serializable* generator() {
		return (Serializable *) new ListLotsRequest();
	}
};


class ListLotsResponse : Body
{
	std::list<LotShortInfo> *lotsInfo;

public:
	static Serializable* generator() {
		return (Serializable *) new ListLotsResponse();
	}
};


class Bet
{
	int customerId;
	int productId;
	int newPrice;
};


class MakeBetRequest : Body
{
	Bet *bet;

public:
	static Serializable* generator() {
		return (Serializable *) new MakeBetRequest();
	}
};


class MakeBetResponse : Body
{
	bool accepted;

public:
	static Serializable* generator() {
		return (Serializable *) new MakeBetResponse();
	}
};


class LotFullInfo
{
	int ownerId;
	int lotId;
	bool opened;
	std::string * description;
	int startPrice;
	std::list<Bet> *bets;
};


class LotDetailsRequest : Body
{
public:
	static Serializable* generator() {
		return (Serializable *) new LotDetailsRequest();
	}
};


class LotDetailsResponse : Body
{
	LotFullInfo *lotDetails;

public:
	static Serializable* generator() {
		return (Serializable *) new LotDetailsResponse();
	}
};


class CloseLotRequest : Body
{
	int lotId;

public:
	static Serializable* generator() {
		return (Serializable *) new CloseLotRequest();
	}
};


class CloseLotResponse : Body
{
	bool closed;

public:
	static Serializable* generator() {
		return (Serializable *) new CloseLotResponse();
	}
};
