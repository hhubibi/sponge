#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!_syn && !seg.header().syn) {
        return;
    }
    if (_fin && _reassembler.empty()) {
        return;
    }

    WrappingInt32 n{0};
    if (seg.header().syn) {
        _syn = true;
        _isn = seg.header().seqno;
        n = _isn + 1;
    } else {
        n = seg.header().seqno;
    }
    
    if (seg.header().fin) {
        _fin = true;
    }
    uint64_t index = unwrap(n, _isn, static_cast<uint64_t>(stream_out().bytes_written()));
    _reassembler.push_substring(seg.payload().copy(), index - 1, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn) {
        return {};
    } else {
        return wrap(static_cast<uint64_t>(stream_out().bytes_written()) + 1 + (_fin && _reassembler.empty()), _isn);
    }
}

size_t TCPReceiver::window_size() const { return stream_out().bytes_read() + _capacity - stream_out().bytes_written(); }
