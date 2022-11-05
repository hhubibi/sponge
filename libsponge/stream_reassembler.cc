#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _unassembled_map() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (_output.input_ended()) {
        return;
    }

    size_t first_unassembled = _output.bytes_written();
    size_t first_unacceptable = _output.bytes_read() + _capacity;

    // Discards bytes that have been assembled. If is eof, set the eof flag.
    if (index + data.length() <= first_unassembled) {
        if (eof) {
            _output.end_input();
        }
        return;
    }

    // Discards bytes whose indice exceed the capacity. 
    if (index >= first_unacceptable) {
        return;
    }

    size_t start = index <= first_unassembled ? first_unassembled : index;
    size_t end = index + data.length() >= first_unacceptable ? first_unacceptable : index + data.length();

    // Truncates the data as it's tail exceed the capacity.
    string new_data = data.substr(start - index, end - index);

    // Set eof index and flag if the data's eof is remained.
    if (eof && end == index + data.length()) {
        _eof_pos = index + data.length();
        _eof_setted = true;
    }

    if (start == first_unassembled) {
        push_string(new_data, start);

        auto it = _unassembled_map.begin();
        while (it != _unassembled_map.end()) {
            first_unassembled = _output.bytes_written();
            if (it->first <= first_unassembled) {
                if (it->second.second > first_unassembled) {
                    size_t tmp_start = it->first <= first_unassembled ? first_unassembled : it->first;
                    push_string(it->second.first.substr(tmp_start - it->first), tmp_start);
                }
                _unassembled -= it->second.first.length();
                it = _unassembled_map.erase(it);
            } else {
                break;
            }
        }
    } else {
        // TODO: insert it to map
        bool need_insert = true;
        auto it = _unassembled_map.begin();
        while (it != _unassembled_map.end()) {
            if (it->first <= start) {
                if (it->second.second >= end) {
                    need_insert = false;
                    break;
                } else {
                    if (it->second.second > start) {
                        _unassembled -= (it->second.second - start);
                        it->second.first = it->second.first.substr(0, start - it->first);
                        it->second.second = start;
                        if (it->second.first.length() == 0) {
                            it = _unassembled_map.erase(it);
                            continue;
                        }
                    }
                    ++it;
                }
            } else {
                if (it->first >= end) {
                    break;
                } else {
                    if (it->second.second <= end) {
                        _unassembled -= it->second.first.length();
                        it = _unassembled_map.erase(it);
                    } else {
                        end = it->first;
                        new_data = new_data.substr(0, end - start);
                        ++it;
                    }
                }
            }
        }
        if (need_insert && new_data.length() > 0) {
            _unassembled_map[start] = make_pair(new_data, end);
            _unassembled += new_data.length();
        }
    }
}

void StreamReassembler::push_string(const string &data, const size_t index) {
    if (_eof_setted && index + data.length() == _eof_pos) {
        _output.end_input();
    }
    _output.write(data);
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled; }

bool StreamReassembler::empty() const { return _unassembled == 0; }
