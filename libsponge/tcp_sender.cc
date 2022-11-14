#include "tcp_sender.hh"

#include "buffer.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    return _bytes_in_flight;
}

void TCPSender::fill_window() {
    if (_fin) {
        return;
    }

    TCPSegment seg;
    if (_next_seqno == 0) {
        seg.header().syn = true;
        send_tcp_segment(seg);
        return;
    }

    uint16_t window_size = _window_size == 0 ? 1 : _window_size;
    if (_stream.eof() && window_size + _ackno > _next_seqno) {
        seg.header().fin = true;
        send_tcp_segment(seg);
        _fin = true;
        return;
    }

    while (!_stream.buffer_empty() && window_size + _ackno > _next_seqno) {
        size_t seg_len = window_size - (_next_seqno - _ackno);
        seg_len = seg_len > _stream.buffer_size() ? _stream.buffer_size() : seg_len;
        seg_len = seg_len > TCPConfig::MAX_PAYLOAD_SIZE ? TCPConfig::MAX_PAYLOAD_SIZE : seg_len;

        seg.payload() = Buffer(_stream.read(seg_len));

        if (_stream.eof() && _next_seqno + seg_len + 1 <= _ackno + window_size) {
            seg.header().fin = true;
            send_tcp_segment(seg);
            _fin = true;
            return;
        }
        send_tcp_segment(seg);
    }
}

void TCPSender::send_tcp_segment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
    _segments_track.push(seg);
    _bytes_in_flight += seg.length_in_sequence_space();
    _next_seqno += seg.length_in_sequence_space();
    if (!_timer.running) {
        _timer.start(_initial_retransmission_timeout);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t orig_ackno = _ackno;

    _ackno = unwrap(ackno, _isn, orig_ackno);
    if (_ackno > _next_seqno) {
        return;
    }
    _window_size = window_size;
    bool new_acked = false;
    while (!_segments_track.empty()) {
        TCPSegment seg = _segments_track.front();
        uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, orig_ackno);
        if (abs_seqno + seg.length_in_sequence_space() <= _ackno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_track.pop();
            new_acked = true;
        } else {
            break;
        }
    }
    if (new_acked) {
        if (!_segments_track.empty()) {
            _timer.start(_initial_retransmission_timeout);
        } else {
            _timer.stop();
        }
        _consecutive_retransmissions = 0;
    }

    if (_window_size > 0) {
        fill_window();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.tick(ms_since_last_tick);

    if (_timer.expire()) {
        if (!_segments_track.empty()) {
            TCPSegment seg = _segments_track.front();
            uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, _ackno);
            if (abs_seqno + seg.length_in_sequence_space() > _ackno) {
                _segments_out.push(seg);
            }

            if (_window_size > 0) {
                _consecutive_retransmissions += 1;
                _timer.start(_timer.timeout * 2);
            } else {
                _timer.start(_timer.timeout);
            }
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmissions;
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().ackno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
