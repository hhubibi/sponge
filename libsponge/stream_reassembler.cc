#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _unassembled_indice(), _unassembled_map() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t first_unread = _output.bytes_read();
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

    // Discards bytes as the `_output` buffer is full or input reaches eof.
    if (_output.remaining_capacity() == 0 || _output.input_ended()) {
        return;
    }

    // Set eof index and flag.
    if (eof) {
        _eof_pos = index + data.length();
        _eof_setted = true;
    }

    if (index <= _output.bytes_written()) {
        
    }


    // TODO:
    // 1. How to use eof ?
    // 2. Push first, then remove some items in map
    // 3. Can't push, insert to map

    

    

    if (index <= _output.bytes_written()) {
        push_substring(data, index);

        while (it != _unassembled_map.end()) {
            if (it->first <= _output.bytes_written()) {
                size_t curr_idx = it->first;
                string curr_data = it->second;
                size_t written = push_substring(it->second, it->first);
                _unassembled -= written;
                it = _unassembled_map.erase(it);
                _unassembled_indice.remove(it->first);
                if (written < it->second.length()) {
                    _unassembled_indice.push_front(curr_idx + written);
                    _unassembled_map[curr_idx] = curr_data.substr(written);
                    break;
                }
            } else {
                break;
            }
        }

        if (_output.buffer_size() + _unassembled >= _capacity) {
            size_t to_remove = _output.buffer_size() + _unassembled - _capacity;
            _unassembled -= to_remove;
            while (!_unassembled_indice.empty() && to_remove > 0) {
                size_t idx = _unassembled_indice.back();
                if (_unassembled_map[idx].length() > to_remove) {
                    size_t truncated_pos = _unassembled_map[idx].length() - to_remove;
                    _unassembled_map[idx] = _unassembled_map[idx].substr(0, truncated_pos);
                    break;
                } else {
                    _unassembled_indice.pop_back();
                    to_remove -= _unassembled_map[idx].length();
                    _unassembled_map.erase(idx);
                }
            }
        } 
        
        
    } else {
        string new_data;
        size_t new_index;
        if (_unassembled_map.count(index)) {
            if (data.length() <= _unassembled_map[index].length()) {
                return;
            } else {
                new_data = data.substr(_unassembled_map[index].length());
                new_index = index + _unassembled_map[index].length();
            }
        } else {
            new_data = data;
            new_index = index;
            size_t begin = 0;
            size_t end = data.length();
            size_t data_end_pos = index + data.length();
            auto it = _unassembled_map.begin();
            while (it != _unassembled_map.end()) {
                if (it->first <= index) {
                    if (it->first + it->second.length() <= index) {
                        continue;
                    } else {
                        if (it->first + it->second->length() >= data_end_pos) {
                            return;
                        } else {
                            if ()
                        }
                    }
                }

                if (it->first > index) {
                    break;
                } else {
                    if (it->first + it->second.length() <= index) {
                        ++it;
                        continue;
                    }
                    if (it->first + it->second.length() >= index + data.length()) {
                        return;
                    } else {
                        new_data = data.substr(it->second.length() + it->first - index);
                        new_index = it->first + it->second.length();
                        break;
                    }
                }
                ++it;
            }
        }
        _unassembled += new_data.size();
        _unassembled_indice.push_back(new_index);
        _unassembled_map[new_index] = new_data; 
    }

    // Discards bytes that exceed the capacity or have been pushed.
}

size_t StreamReassembler::push_substring(const string &data, const size_t index) {
    // The end index of the data string within limits of the capacity
    size_t first_unassembled = _output.bytes_written();
    size_t next_unassembled = index + data.length();
    size_t remained_cap = _capacity - _output.buffer_size();
    
    if (next_unassembled - first_unassembled > remained_cap) {
        next_unassembled = first_unassembled + remained_cap;
    }
    if (_eof_setted && _eof_pos <= next_unassembled) {
        _output.end_input();
        next_unassembled = _eof_pos;
    }
    if (first_unassembled - index >= data.length()|| first_unassembled >= next_unassembled) {
        return data.length();
    }
    _output.write(data.substr(first_unassembled - index, next_unassembled - first_unassembled));
    return next_unassembled + index - 2 * first_unassembled;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled; }

bool StreamReassembler::empty() const { return _unassembled == 0; }
