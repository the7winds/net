#include "protocol.h"
#include "util.h"
#include <memory>


static std::map<Body::BodyType, Serializable *(*)()> constructorTable =
		{
				{Body::BodyType::AUTH_RESP,      &AuthorisationResponse::generator},
				{Body::BodyType::LIST_LOTS_REQ,  &ListLotsRequest::generator},
				{Body::BodyType::LIST_LOTS_RESP, &ListLotsResponse::generator},
				{Body::BodyType::LOT_DET_REQ,    &LotDetailsRequest::generator},
				{Body::BodyType::LOT_DET_RESP,   &LotDetailsResponse::generator},
				{Body::BodyType::CLOSE_LOT_REQ,  &CloseLotRequest::generator},
				{Body::BodyType::CLOSE_LOT_RESP, &CloseLotResponse::generator}
		};


void Packet::writeToStreamSocket(stream_socket *sk)
{
	send_uint((uint32_t) body->getType(), sk);
	body->writeToStreamSocket(sk);
}


void Packet::readFromStreamSocket(stream_socket *sk)
{
	uint32_t t32;

	recv_uint(t32, sk);
	Body::BodyType bodyType = (Body::BodyType) t32;

	body = constructorTable[bodyType]();
	body->readFromStreamSocket(sk);
}


void AuthorisationResponse::writeToStreamSocket(stream_socket *sk)
{
	send_uint(customerId, sk);
}


void AuthorisationResponse::readFromStreamSocket(stream_socket *sk)
{
	recv_uint(customerId, sk);
}


void NewLotRequest::writeToStreamSocket(stream_socket *sk)
{
	send_string(description, sk);
	send_uint(startPrice, sk);
}


void NewLotRequest::readFromStreamSocket(stream_socket *sk)
{
	description = recv_string(sk);
	recv_uint(startPrice, sk);
}


void NewLotResponse::writeToStreamSocket(stream_socket *sk)
{
	send_bool(accepted, sk);
}


void NewLotResponse::readFromStreamSocket(stream_socket *sk)
{
	recv_bool(accepted, sk);
}


void ListLotsResponse::writeToStreamSocket(stream_socket *sk)
{
	send_uint((uint32_t) lotsInfo.size(), sk);

	for (auto i = lotsInfo.begin(); i != lotsInfo.end(); ++i) {
		send_uint(i.->lotId, sk);
		send_bool(i.->opened, sk);
		send_uint(i.->startPrice, sk);
		send_uint(i.->bestPrice, sk);
		send_string(i.->description, sk);
	}
}


void ListLotsResponse::readFromStreamSocket(stream_socket *sk)
{
	uint32_t lotsInfoSize;

	recv_uint(lotsInfoSize, sk);

	uint32_t lotId;
	bool opened;
	uint32_t startPrice;
	uint32_t bestPrice;
	std::string description;

	for (int i = 0; i < lotsInfoSize; ++i) {
		recv_uint(lotId, sk);
		recv_bool(opened, sk);
		recv_uint(startPrice, sk);
		recv_uint(bestPrice, sk);
		description = recv_string(sk);

		lotsInfo.push_back(LotShortInfo(lotId, opened, startPrice, bestPrice, description));
	}

}


void MakeBetRequest::writeToStreamSocket(stream_socket *sk)
{
	send_uint(bet.productId, sk);
	send_uint(bet.customerId, sk);
	send_uint(bet.newPrice, sk);
}


void MakeBetRequest::readFromStreamSocket(stream_socket *sk)
{
	recv_uint(bet.productId, sk);
	recv_uint(bet.customerId, sk);
	recv_uint(bet.newPrice, sk);
}


void MakeBetResponse::writeToStreamSocket(stream_socket *sk)
{
	send_bool(accepted, sk);
}


void MakeBetResponse::readFromStreamSocket(stream_socket *sk)
{
	recv_bool(accepted, sk);
}


void LotDetailsResponse::writeToStreamSocket(stream_socket *sk)
{
	send_uint(lotDetails.lotId, sk);
	send_bool(lotDetails.opened, sk);
	send_uint(lotDetails.ownerId, sk);
	send_uint(lotDetails.startPrice, sk);
	send_string(lotDetails.description, sk);

	send_uint((uint32_t) lotDetails.bets.size(), sk);
	for (auto i = lotDetails.bets.begin(); i != lotDetails.bets.end(); ++i) {
		send_uint(i->customerId, sk);
		send_uint(i->newPrice, sk);
	}
}


void LotDetailsResponse::readFromStreamSocket(stream_socket *sk)
{
	uint32_t lotId;
	bool opened;
	uint32_t ownerId;
	uint32_t startPrice;
	std::string description;

	recv_uint(lotId, sk);
	recv_bool(opened, sk);
	recv_uint(ownerId, sk);
	recv_uint(startPrice, sk);
	description = recv_string(sk);

	uint32_t betsLen;
	recv_uint(betsLen, sk);

	std::list<Bet> bets;
	uint32_t customerId;
	uint32_t newPrice;
	for (uint32_t i = 0; i < betsLen; ++i) {
		recv_uint(customerId, sk);
		recv_uint(newPrice, sk);
		bets.push_back(Bet(lotId, customerId, newPrice));
	}

	lotDetails = LotFullInfo(lotId, ownerId, opened, description, startPrice, bets);
}


void CloseLotRequest::writeToStreamSocket(stream_socket *sk)
{
	send_uint(lotId, sk);
}


void CloseLotRequest::readFromStreamSocket(stream_socket *sk)
{
	recv_uint(lotId, sk);
}


void CloseLotResponse::writeToStreamSocket(stream_socket *sk)
{
	send_bool(closed, sk);
}


void CloseLotResponse::readFromStreamSocket(stream_socket *sk)
{
	send_bool(closed, sk);
}