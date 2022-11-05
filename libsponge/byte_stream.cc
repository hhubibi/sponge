#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity),
            _first(capacity - 1), _buffer() {
    _buffer.resize(capacity);
}

size_t ByteStream::write(const string &data) {
    size_t remaining = remaining_capacity();
    size_t can_write = data.length() > remaining ? remaining : data.length();
    int next_last = _plus_num(_last, static_cast<int>(can_write));
    int end_pos = _minus_num(next_last, 1);
    if (end_pos < _last) {
        copy(data.begin(), data.begin() + _capacity - _last, _buffer.begin() + _last);
        copy(data.begin() + _capacity - _last, data.begin() + can_write, _buffer.begin());
    } else {
        copy(data.begin(), data.begin() + can_write, _buffer.begin() + _last);
    }
    _last = next_last;
    _total_write += can_write;
    _size += can_write;
    return can_write;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    if (len == 0) {
        return "";
    }
    size_t can_peek = len > _size ? _size : len;
    string out_buffer(can_peek, ' ');
    int next_first = _plus_num(_first, static_cast<int>(can_peek));
    int start_pos = _plus_num(_first, 1);
    if (next_first < start_pos) {
        copy(_buffer.begin() + start_pos, _buffer.end(), out_buffer.begin());
        copy(_buffer.begin(), _buffer.begin() + next_first + 1, out_buffer.begin() + _capacity - start_pos);
    } else {
        copy(_buffer.begin() + start_pos, _buffer.begin() + next_first + 1, out_buffer.begin());
    }
    return out_buffer;
}

//! \param[in] len bytes will be removed from the output side of the _buffer
void ByteStream::pop_output(const size_t len) {
    size_t can_pop = len > _size ? _size : len;
    _first = _plus_num(_first, static_cast<int>(can_pop));
    _size -= can_pop;
    _total_read += can_pop;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string out_buffer = peek_output(len);
    _first = _plus_num(_first, out_buffer.length());
    _total_read += out_buffer.length();
    _size -= out_buffer.length();
    return out_buffer;
}

void ByteStream::end_input() {
    _eof = true;
}

bool ByteStream::input_ended() const { return _eof; }

size_t ByteStream::buffer_size() const { return _size; }

bool ByteStream::buffer_empty() const { return _size == 0; }

bool ByteStream::eof() const { return _eof && _size == 0; }

size_t ByteStream::bytes_written() const { return _total_write; }

size_t ByteStream::bytes_read() const { return _total_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _size; }
