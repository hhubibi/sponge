#include "stream_reassembler.hh"
#include <cstddef>
#include <algorithm>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        _eof_index = index + data.length();
        _eof = true;
    }

    size_t first_unassemble = _output.bytes_written();
    size_t first_unacceptable = _output.bytes_read() + _capacity;
    if (index + data.length() <= first_unassemble || index >= first_unacceptable) {
        if (_eof_index <= first_unassemble && _eof) {
            _output.end_input();
        }
        return;
    }

    size_t start = index < first_unassemble ? first_unassemble : index;
    size_t end = index + data.length() > first_unacceptable ? first_unacceptable : index + data.length();
    string new_data = data.substr(start - index, end - index);

    auto it = _buffer.begin();
    while (it != _buffer.end()) {
        size_t left = it->first;
        size_t right = it->first + it->second.length();
        if (end <= left) {
            break;
        } else if (start >= right) {
            it++;
            continue;
        } else {
            string tmp = "";
            if (start < left) {
                tmp += new_data.substr(0, left - start);
            }
            tmp += it->second;
            if (end > right) {
                tmp += new_data.substr(right - start, end - start);
            }
            _unassemble -= it->second.length();
            start = min(start, left);
            end = max(end, right);
            new_data = tmp;
            it = _buffer.erase(it);
        }
    }
    _buffer[start] = new_data;
    _unassemble += new_data.length();

    it = _buffer.begin();
    while (it != _buffer.end()) {
        if (it->first != _output.bytes_written()) {
            break;
        }
        _output.write(it->second);
        _unassemble -= it->second.length();
        it = _buffer.erase(it);
    }
    if (_output.bytes_written() == _eof_index && _eof) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassemble; }

bool StreamReassembler::empty() const { return _unassemble == 0; }
