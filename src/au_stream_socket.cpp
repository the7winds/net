#include "au_stream_socket.h"
#include "util.h"
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <sys/timerfd.h>
#include <cstring>
#include <iostream>
#include <exception>
#include <boost/crc.hpp>
#include <condition_variable>

/* zero filled packets */
static ip_packet z_ip;
static tcp_packet z_tcp;

template <uint16_t buf_size>
uint16_t send_buffer<buf_size>::write(const void* data, uint16_t l) {
    std::lock_guard<std::mutex> lock(mutex);
    const char* cdata = (const char*) data;

    uint16_t copy = std::min(l, (uint16_t) (buf_size - len_total));
    for (uint16_t i = 0; i < copy; ++i) {
        buf[(begin + len_total) % buf_size] = cdata[i];
        ++len_total;
    }

    return copy;
}


template <uint16_t buf_size>
uint16_t send_buffer<buf_size>::copy_for_transmit(char *data, uint16_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    uint16_t copy = std::min(size, (uint16_t) (len_total - len_sent));

    for (uint16_t i = 0; i < copy; ++i) {
        data[i] = buf[(begin + len_sent + i) % buf_size];
    }

    len_sent += copy;

    return copy;
}


template <uint16_t buf_size>
uint16_t send_buffer<buf_size>::copy_for_retransmit(char *data, uint16_t size) {
    std::lock_guard<std::mutex> lock(mutex);

    uint16_t copy = std::min(size, len_sent);

    for (uint16_t i = 0; i < copy; ++i) {
        data[i] = buf[(begin + i) % buf_size];
    }

    return copy;
}


template <uint16_t buf_size>
void send_buffer<buf_size>::free_to_pos(uint32_t pos) {
    std::lock_guard<std::mutex> lock(mutex);
    if (pos < begin) {
        return;
    }

    uint16_t delta = pos - begin;
    begin += delta;
    len_total -= delta;
    if (delta > len_sent) {
        len_sent = 0;
    } else {
        len_sent -= delta;
    }

    if (len_sent == 0) {
        retransmit = false;
    }
}


template <uint16_t buf_size>
uint16_t recv_buffer<buf_size>::write(const void* data, uint16_t l) {
    std::lock_guard<std::mutex> lock(mutex);
    const char* cdata = (const char*) data;

    uint16_t copy = std::min(l, (uint16_t) (buf_size - len));
    for (uint16_t i = 0; i < copy; ++i) {
        buf[(begin + len) % buf_size] = cdata[i];
        ++len;
    }

    return copy;
}


template <uint16_t buf_size>
uint16_t recv_buffer<buf_size>::read(void* data, uint16_t l) {
    std::lock_guard<std::mutex> lock(mutex);
    char* cdata = (char*) data;

    uint16_t copy = std::min(l, len);
    for (uint16_t i = 0; i < copy; ++i) {
        cdata[i] = buf[begin % buf_size];
        begin += 1;
        len -= 1;
    }

    return copy;
}

void tcp_packet::set_crc16(tcp_packet& packet, size_t len) {
    boost::crc_16_type res;
    packet.header.check = 0;
    res.process_bytes(packet.raw, len);
    packet.header.check = res.checksum();
}

bool tcp_packet::has_errors(tcp_packet& packet, size_t len) {
    u_int16_t crc = packet.header.check;
    tcp_packet::set_crc16(packet, len);
    bool err = crc != packet.header.check;
    packet.header.check = crc;
    return err;
}

int open_socket() {
    int sk = socket(AF_INET, SOCK_RAW, IPPROTO_AU_TCP);
    if (sk < 0) {
        throw std::runtime_error("can't open socket");
    }

    return sk;
}


/* au_stream_socket implementation */

void au_stream_socket::send(const void *data, size_t size) {
    const char* cdata = (const char*) data;
    size_t writtern_bytes = 0;

    logger.info("%d: call send", connection.my_port);

    while (writtern_bytes != size) {
        if (state.load() != TCP_ESTABLISHED)
            throw std::runtime_error("invalid state");

        writtern_bytes += send_buf.write(cdata + writtern_bytes, size - writtern_bytes);

        if (send_buf.is_full()) {
            flush();
        }
    }

    while (!send_buf.is_empty()) {
        flush();
    }
}

/* retransmits the oldest segment */
void au_stream_socket::retransmit() {
    tcp_packet send_packet = z_tcp;

    if (!send_buf.need_retransmit())
        return;

    uint16_t max_send_size = std::min((uint16_t) window.load(), (uint16_t) TCP_MAX_SEG);

    send_packet.header.seq = send_buf.get_seq();
    uint16_t copied = std::max((uint16_t) 1, send_buf.copy_for_retransmit(send_packet.data, max_send_size));
    send_buf.set_retransmit(false);

    // TODO do something with zero
    if (copied) {
        try_send(sk_send, send_packet, sizeof(tcphdr) + copied);
        logger.debug("%d:%d: retransmit seq %d size %d",
                     connection.my_port, connection.other_port, send_packet.header.seq, copied);
    } else {
        return;
    }
}

/* first sending data */
void au_stream_socket::transmit() {
    tcp_packet send_packet = z_tcp;

    while (send_buf.contains_not_send()) {
        uint16_t max_send_size = std::max((uint16_t) 1, std::min((uint16_t) window.load(), (uint16_t) TCP_MAX_SEG));

        send_packet.header.seq = send_buf.get_sent_seq();
        int copied = send_buf.copy_for_transmit(send_packet.data, max_send_size);

        // TODO do something with zero
        if (copied) {
            try_send(sk_send, send_packet, sizeof(tcphdr) + copied);
            logger.debug("%d:%d: transmit seq %d size %d window %d",
                         connection.my_port, connection.other_port, send_packet.header.seq, copied, window.load());
        } else {
            return;
        }
    }
}

void au_stream_socket::flush() {
    logger.debug("%d:%d: call flush", connection.my_port, connection.other_port);

    if (!send_buf.is_empty()) {
        transmit();

        std::unique_lock<std::mutex> rto_unique_lock(rto_lock);
        std::cv_status status = rto_condition.wait_for(rto_unique_lock, RTO);
        if (status == std::cv_status::timeout) {
            send_buf.set_retransmit(true);
        }

        retransmit();
    }
}


void au_stream_socket::try_send(int sk, tcp_packet& packet, size_t len) {
    packet.header.source = connection.my_port;
    packet.header.dest = connection.other_port;
    packet.header.window = recv_buf.spare();
    tcp_packet::set_crc16(packet, len);

    for (int lim = 5; lim; --lim) {
        if (sendto(sk, &packet, len, 0, reinterpret_cast<sockaddr *>(&connection.addr), sizeof(sockaddr_in)) == len) {
            return;
        }
    }

    state.store(TCP_CLOSE);
    throw std::runtime_error("can't send packet");
}


void au_stream_socket::handle_ack(tcp_packet& packet) {
    prev_ack = 0;

    logger.debug("%d:%d: handle ack", connection.my_port, connection.other_port);

    if (packet.header.ack_seq >= send_buf.get_seq()) {
        if (send_buf.get_seq() == packet.header.ack_seq) {
            logger.debug("%d:%d: set fast retransmit (window %d)",
                         connection.my_port, connection.other_port, packet.header.window);
            if (prev_ack == 3) {
                send_buf.set_retransmit(true);
            }
        } else if (packet.header.ack_seq > send_buf.get_seq()) {
            logger.debug("%d:%d: received ack %d (window %d)",
                         connection.my_port, connection.other_port, packet.header.ack_seq, packet.header.window);
            prev_ack = 0;
            send_buf.free_to_pos(packet.header.ack_seq);
        }
        window.store(packet.header.window);
        rto_condition.notify_all();
    } else {
        logger.debug("%d:%d: received old ack %d", connection.my_port, connection.other_port, packet.header.ack_seq);
    }

    // case of ack_seq < my seq is ignored it's arrived too late
}

// TODO
void au_stream_socket::handle_data(tcp_packet& packet, size_t data_size) {
    tcp_packet ack = z_tcp;

    logger.debug("%d:%d: handle data seq %d wait seq %d size %d window %d",
                 connection.my_port, connection.other_port, packet.header.seq, recv_buf.get_wait_seq(), data_size, packet.header.window);

    if (packet.header.seq == recv_buf.get_wait_seq()) {
        recv_buf.write(packet.data, data_size);
        window.store(packet.header.window);
    }

    ack.header.ack = 1;
    ack.header.ack_seq = recv_buf.get_wait_seq();

    try_send(sk_recv, ack, sizeof(tcphdr));
    logger.debug("%d:%d: handle data send ack %d", connection.my_port, connection.other_port, ack.header.ack_seq);
}

void au_stream_socket::handle_fin(tcp_packet& packet) {
    // TODO
    logger.debug("handle fin");
    state.store(TCP_CLOSE);
}

void au_stream_socket::listen(au_stream_socket* self) {
    ip_packet recv_packet = z_ip;

    timeval wait_recv_to = {
            .tv_sec = 0,
            .tv_usec = 100000
    };

    if (setsockopt(self->sk_recv, SOL_SOCKET, SO_RCVTIMEO, &wait_recv_to, sizeof(timeval))) {
        self->logger.info("errno %d", errno);
        throw std::runtime_error("can't configure socket");
    }

    self->state.store(TCP_ESTABLISHED);

    self->logger.info("%d: start listen", self->connection.my_port);

    while (self->state.load() == TCP_ESTABLISHED) {
        ssize_t sz = recvfrom(self->sk_recv, &recv_packet, sizeof(recv_packet), MSG_DONTWAIT, NULL, 0);

        if (sz < 0) {
            if (errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EINTR) {
                continue;
            } else {
                self->state.store(TCP_CLOSE);
                self->logger.debug("errno %d", errno);
                throw std::runtime_error("fail");
            }
        }

        if (tcp_packet::has_errors(recv_packet.tcp, sz - sizeof(iphdr)))
            continue;

        if (!self->connection.check_income_packet(recv_packet))
            continue;

        ssize_t data_size = sz - sizeof(iphdr) - sizeof(tcphdr);

        if (recv_packet.tcp.header.ack == 1) {
            self->handle_ack(recv_packet.tcp);
        }

        if (data_size > 0) {
            self->handle_data(recv_packet.tcp, data_size);
        }

        if (recv_packet.tcp.header.fin == 1) {
            self->handle_fin(recv_packet.tcp);
        }
    }

    self->logger.info("%d: listen finished", self->connection.my_port);
}


void au_stream_socket::recv(void *buf, size_t size) {
    char* cbuf = (char*) buf;
    int read_bytes = 0;

    logger.info("%d: call recv", connection.my_port);

    while (read_bytes < size) {
        read_bytes += recv_buf.read(cbuf + read_bytes, size - read_bytes);

        if (state.load() != TCP_ESTABLISHED) {
            throw std::runtime_error("invalid state");
        }
    }
}


/* au_stream_client_socket implementation */

au_stream_client_socket::au_stream_client_socket(const char *addr, au_port client_port, au_port server_port) {
    sk.sk_send = open_socket();
    sk.sk_recv = open_socket();

    sk.connection.other_port = server_port;
    init_ipv4addr(addr, server_port, sk.connection.addr);
    sk.connection.other_ip = sk.connection.addr.sin_addr.s_addr;
    sk.connection.my_port = client_port;
}

void au_stream_client_socket::connect() {
    tcp_packet send_packet = z_tcp;
    ip_packet recv_packet = z_ip;

    if (sk.state.load() != TCP_CLOSE)
        return;

    send_packet.header.source = sk.connection.my_port;
    send_packet.header.dest = sk.connection.other_port;
    send_packet.header.syn = 1;
    send_packet.header.seq = sk.send_buf.get_seq();

    try {
        sk.try_send(sk.sk_send, send_packet, sizeof(tcphdr));
    } catch (std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + "\n\t" +  "can't start handshake");
    }
    sk.logger.info("client send syn");

    while (true) {
        if (recvfrom(sk.sk_recv, &recv_packet, sizeof(recv_packet), 0, NULL, 0) < 0) {
            perror(0);
            throw std::runtime_error("can't recv syn-ack");
        }

        if (sk.connection.check_income_packet(recv_packet)
            && recv_packet.tcp.header.ack
            && recv_packet.tcp.header.syn)
            break;
    }
    sk.logger.info("client received ack-syn");

    sk.recv_buf.init(recv_packet.tcp.header.seq + 1);
    sk.send_buf.init(recv_packet.tcp.header.ack_seq);

    send_packet.header.ack = 1;
    send_packet.header.ack_seq = sk.recv_buf.get_wait_seq();

    try {
        sk.try_send(sk.sk_send, send_packet, sizeof(tcphdr));
    } catch (std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + "\n\t" +  "can't send ack");
    }
    sk.logger.info("client send ack");

    sk.start();
}


/* au_stream_server_socket */

au_stream_server_socket::au_stream_server_socket(const char *addr, au_port port) {
    sk = open_socket();

    connection.my_port = port;
    init_ipv4addr(addr, port, connection.addr);
    connection.my_ip = connection.addr.sin_addr.s_addr;

    if (bind(sk, reinterpret_cast<sockaddr*>(&connection.addr), sizeof(sockaddr_in)) < 0) {
        throw std::runtime_error("can't bind socket");
    }
}

stream_socket* au_stream_server_socket::accept_one_client() {
    tcp_packet send_packet = z_tcp;
    ip_packet recv_packet = z_ip;
    socklen_t addr_len = sizeof(sockaddr_in);

    while (true) {
        if (recvfrom(sk, reinterpret_cast<sockaddr*>(&recv_packet), sizeof(recv_packet), 0, reinterpret_cast<sockaddr*>(&connection.addr), &addr_len) < 0) {
            if (errno == EAGAIN) {
                continue;
            }

            throw std::runtime_error("can't recv syn");
        }
        if (recv_packet.header.daddr == connection.my_ip
            && recv_packet.tcp.header.dest == connection.my_port
            && recv_packet.tcp.header.syn)
            break;
    }

    logger.info("server received syn");

    au_stream_socket* client = new au_stream_socket();

    client->sk_send = open_socket();
    client->sk_recv = open_socket();

    connection.other_ip = connection.addr.sin_addr.s_addr;
    connection.other_port = recv_packet.tcp.header.source;
    client->connection = connection;
    client->recv_buf.init(recv_packet.tcp.header.seq + 1);

    send_packet.header.syn = 1;
    send_packet.header.seq = client->send_buf.get_seq();
    send_packet.header.ack = 1;
    send_packet.header.ack_seq = client->recv_buf.get_wait_seq();

    try {
        client->try_send(client->sk_send, send_packet, sizeof(tcphdr));
    } catch (std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + "\n\t" +  "can't finish handshake");
    }

    logger.info("server sent syn-ack");

    while (true) {
        if (recvfrom(sk, reinterpret_cast<sockaddr*>(&recv_packet), sizeof(recv_packet), 0, reinterpret_cast<sockaddr*>(&client->connection.addr), &addr_len) < 0)
            throw std::runtime_error("can't receive ack");
        if (recv_packet.tcp.header.dest == connection.my_port && recv_packet.tcp.header.ack)
            break;
    }
    logger.info("server recveived ack");

    client->send_buf.init(recv_packet.tcp.header.ack_seq);
    client->start();

    return static_cast<stream_socket*>(client);
}