#pragma once

#include "stream_socket.h"
#include <string>

void send_string(std::string& str, stream_socket& sk);
std::string recv_string(stream_socket& sk);

void send_uint(uint32_t x, stream_socket sk);
void recv_uint(uint32_t& x, stream_socket sk);

void send_uint(uint16_t x, stream_socket sk);
void recv_uint(uint16_t& x, stream_socket sk);

void send_uint(uint8_t x, stream_socket sk);
void recv_uint(uint8_t& x, stream_socket sk);

void send_bool(bool x, stream_socket &sk);
void recv_bool(bool &x, stream_socket &sk);