#include "tcp_connection.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "tcp_state.hh"
#include "wrapping_integers.hh"

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!_active) {
        return;
    }
    _time_since_last_segment_received = 0;

    // State -> RESET
    if (seg.header().rst) {
        set_rst_state();
        return;
    }

    _receiver.segment_received(seg);

    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        set_outbound_queue();
    }

    if (seg.length_in_sequence_space() > 0) {
        _sender.fill_window();
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        } 
        set_outbound_queue();
        // else {
        //     std::cerr << "return SYN: " << seg.header().syn
        //     << " ACK: " << seg.header().ack
        //     << " FIN: " << seg.header().fin
        //     << " RST: " << seg.header().rst
        //     << " ackno: " << seg.header().ackno
        //     << " seqno: " << seg.header().seqno
        //     << " byte in flight: " << bytes_in_flight()
        //     << " unassembled: " << unassembled_bytes()
        //     << "\nState: " << TCPState(_sender, _receiver, _active, _linger_after_streams_finish).name()
        //     << std::endl;
        //     return;
        // }
    }


    // CLOSE_WAIT, receive FIN
    // if (unassembled_bytes() == 0 && _receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
    //     _linger_after_streams_finish = false;
    // }

    std::cerr << "SYN: " << seg.header().syn
                << " ACK: " << seg.header().ack
                << " FIN: " << seg.header().fin
                << " RST: " << seg.header().rst
                << " ackno: " << seg.header().ackno
                << " seqno: " << seg.header().seqno
                << " byte in flight: " << bytes_in_flight()
                << " unassembled: " << unassembled_bytes()
                << "\nState: " << TCPState(_sender, _receiver, _active, _linger_after_streams_finish).name()
                << std::endl;
}

bool TCPConnection::active() const {
    return _active;
}

size_t TCPConnection::write(const string &data) {
    size_t bytes_written = _sender.stream_in().write(data);
    _sender.fill_window();
    set_outbound_queue();
    return bytes_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        while (!_sender.segments_out().empty()) {
            _sender.segments_out().pop();
        }
        _sender.send_empty_segment();
        set_rst_state();
        return;
    }

    set_outbound_queue();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    // Send FIN
    _sender.fill_window();
    set_outbound_queue();
}

void TCPConnection::connect() {
    // Send SYN, State -> SYN_SENT
    _sender.fill_window();
    set_outbound_queue();
    _active = true;
}

void TCPConnection::set_outbound_queue() {
    bool have_ack = _receiver.ackno().has_value();
    WrappingInt32 ackno = have_ack ? _receiver.ackno().value() : WrappingInt32{0};
    uint16_t win = have_ack ? static_cast<uint16_t>(_receiver.window_size()) : 0;
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        if (have_ack) {
            seg.header().ack = true;
            seg.header().ackno = ackno;
            seg.header().win = win;
        }
        _sender.segments_out().pop();
        _segments_out.push(seg);
    }
    // Preq 1: The inbound stream has been fully assembled and has ended
    if (unassembled_bytes() == 0 && _receiver.stream_out().input_ended()) {
        if (!_sender.stream_in().eof()) {
            _linger_after_streams_finish = false;
        }
        // Preq 2: The outbound stream has been ended
        if (_sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2
                && _sender.bytes_in_flight() == 0) {  // Preq 3: The outbound has been fully acked
            if (!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
                std::cerr << "not active" << std::endl;
                _active = false;
            }
        }
    }
}

void TCPConnection::set_rst_state() {
    if (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        seg.header().rst = true;
        _sender.segments_out().pop();
        _segments_out.push(seg);
    }
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment();
            set_rst_state();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
