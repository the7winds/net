#pragma once

#include <stdint.h>
#include "stream_socket.h"
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <array>
#include <tuple>
#include <cstring>
#include <atomic>
#include <mutex>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <cassert>
#include <stdarg.h>

/**
 * Optimizations:
 * - cumulative ACK
 * - fast retransmit
 * - flow control
 */

constexpr int IPPROTO_AU_TCP = 42;

typedef uint16_t au_port;
typedef const char *hostname;


union tcp_packet {
    char raw[IP_MAXPACKET - sizeof(iphdr)];
    struct {
        tcphdr header;
        char data[IP_MAXPACKET - sizeof(iphdr) - sizeof(tcphdr)];
    };

    static void set_crc16(tcp_packet& packet, size_t len);
    static bool has_errors(tcp_packet& packet, size_t len);
} __attribute__((__packed__));


union ip_packet {
    char raw[IP_MAXPACKET];
    struct {
        iphdr header;
        tcp_packet tcp;
    };
} __attribute__((__packed__));


struct connection_t {
    uint32_t my_ip = 0;
    au_port my_port = 0;
    uint32_t other_ip = 0;
    au_port  other_port = 0;
    sockaddr_in addr = {0};

    bool check_income_packet(ip_packet& packet) {
        return (packet.header.saddr == other_ip
                && packet.tcp.header.source == other_port
                && packet.tcp.header.dest == my_port);
    }
};

int open_socket();

class timer {
    uint64_t lim;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
public:
    timer(uint64_t u_lim) {
        lim = u_lim;
    }

    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }

    bool is_expired() {
        auto ticks = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
        return ticks > lim;
    }
};

class au_logger {
    void log_line(const std::string& format, ...) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, (format + "\n").c_str(), args);
        va_end(args);
    }

public:
    void info(const std::string& format, ...) {
#ifdef AU_INFO
        va_list args;
        va_start(args, format);
        vfprintf(stderr, (format + "\n").c_str(), args);
        va_end(args);
#endif
    }

    void debug(const std::string& format, ...) {
#ifdef AU_DEBUG
        va_list args;
        va_start(args, format);
        vfprintf(stderr, (format + "\n").c_str(), args);
        va_end(args);
#endif
    }
};

template <uint16_t buf_size>
class send_buffer {
    std::mutex mutex;

    std::array<char, buf_size> buf;
    uint32_t begin = 0;
    uint16_t len_sent = 0;
    uint16_t len_total = 0;
    bool retransmit = false;
    bool frozen;

public:
    send_buffer() {
        frozen = false;
    }

    void froze() {
        std::lock_guard<std::mutex> lock(mutex);
        frozen = true;
    }

    void init(uint32_t ack_seq) {
        begin = ack_seq;
    }

    uint16_t write(const void* data, uint16_t len);

    void free_to_pos(uint32_t pos);

    void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        len_sent = 0;
        retransmit = false;
    }

    bool is_empty() {
        std::lock_guard<std::mutex> lock(mutex);
        return len_total == 0;
    }

    bool contains_not_send() {
        std::lock_guard<std::mutex> lock(mutex);
        return len_total - len_sent > 0;
    }

    uint16_t copy_for_transmit(char *data, uint16_t size);

    bool is_full() {
        std::lock_guard<std::mutex> lock(mutex);
        return len_total == buf_size;
    }

    uint32_t get_seq() {
        std::lock_guard<std::mutex> lock(mutex);
        return begin;
    }

    uint32_t get_sent_seq() {
        std::lock_guard<std::mutex> lock(mutex);
        return begin + len_sent;
    }

    bool need_retransmit() {
        std::lock_guard<std::mutex> lock(mutex);
        return retransmit;
    }

    void set_retransmit(bool need) {
        std::lock_guard<std::mutex> lock(mutex);
        if (frozen) {
            return;
        }

        retransmit = need;
    }
};


template <uint16_t buf_size>
struct recv_buffer {
    std::mutex mutex;

    std::array<char, buf_size> buf;
    uint32_t begin = 0;
    uint16_t len = 0;
    std::atomic_bool frozen;

public:
    recv_buffer() {
        frozen.store(false);
    }

    void init(uint32_t wait_seq) {
        begin = wait_seq;
    }

    void froze() {
        std::lock_guard<std::mutex> lock(mutex);
        frozen.store(true);
    }

    uint16_t write(const void* data, uint16_t len);
    uint16_t read(void* data, uint16_t len);

    uint32_t get_wait_seq() {
        std::lock_guard<std::mutex> lock(mutex);
        return begin + len;
    }

    uint16_t spare() {
        std::lock_guard<std::mutex> lock(mutex);
        return buf_size - len;
    }
};


class au_stream_server_socket : public stream_server_socket {
    int sk;
    au_logger logger;
    connection_t connection;

public:
    au_stream_server_socket(const char *addr, au_port port);
    stream_socket *accept_one_client() override;
    ~au_stream_server_socket() {
        logger.info("close server socket");
        close(sk);
    };
};

class au_stream_socket : public stream_socket {
    au_logger logger;

    int sk_send;
    int sk_recv;

    connection_t connection;
    std::thread listen_thread;

    std::atomic_int state;

    void static listen(au_stream_socket* self);
    int prev_ack = 0;
    void handle_ack(tcp_packet& packet);
    void handle_data(tcp_packet& packet, size_t data_size);
    void handle_fin(tcp_packet& packet);

    void try_send(int sk, tcp_packet& packet, size_t len);

    static constexpr uint16_t TCP_MAX_SEG = 1000;
    static constexpr uint16_t BUF_SIZE = 3 * TCP_MAX_SEG;

    send_buffer<BUF_SIZE> send_buf;
    recv_buffer<BUF_SIZE> recv_buf;

    std::atomic<uint16_t> window;

    std::chrono::duration<int64_t> RTO = std::chrono::seconds(3);
    std::mutex rto_lock;
    std::condition_variable rto_condition;

    void retransmit();
    void transmit();
    void flush();

    void start() {
        listen_thread = std::thread(listen, this);
        while (state.load() != TCP_ESTABLISHED);
    }

    void fin();

public:
    au_stream_socket() {
        state.store(TCP_CLOSE);
        window.store(BUF_SIZE);
    }

    void send(const void *buf, size_t size) override;
    void recv(void *buf, size_t size) override;
    ~au_stream_socket() {
        if (state.load() == TCP_ESTABLISHED) {
            fin();
        }

        if (listen_thread.joinable()) {
            listen_thread.join();
        }

        close(sk_send);
        close(sk_recv);
    };

    friend class au_stream_client_socket;
    friend class au_stream_server_socket;
};

class au_stream_client_socket : public stream_client_socket {
    au_stream_socket sk;
public:
    au_stream_client_socket(const char *addr, tcp_port client_port, tcp_port server_port);
    void connect() override;
    void send(const void *buf, size_t size) override {
        sk.send(buf, size);
    }
    void recv(void *buf, size_t size) override { sk.recv(buf, size); }
    ~au_stream_client_socket() {}
};
