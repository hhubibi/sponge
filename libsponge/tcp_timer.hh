#ifndef SPONGE_LIBSPONGE_TCP_TIMER_HH
#define SPONGE_LIBSPONGE_TCP_TIMER_HH

#include <cstdint>

struct TCPTimer {
    unsigned int timeout = 0;
    unsigned int elasped = 0;
    bool running = false;

    TCPTimer() {}

    void start(unsigned int upper_time) {
        running = true;
        elasped = 0;
        timeout = upper_time;
    }

    void tick(const std::size_t ms_since_last_tick) {
        elasped += ms_since_last_tick;
    }

    void stop() {
        running = false;
    }

    bool expire() {
        return elasped >= timeout;
    }
};

#endif