#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _eof(false), _buffer(capacity), _assembled_index(0), _actual_size(capacity), _buffer_size(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t st = max(index, _assembled_index);
    size_t ed = min(index + data.size(), _actual_size);
    if (index + data.size() >= _capacity) {
        _eof = true;
    }
    if (eof && !_eof) {
        _eof = true;
        _actual_size = min(_actual_size, index + data.size());
    }
    for (size_t i = st; i < ed; i++) {
        if (!_buffer[i].second) _buffer_size++;
        _buffer[i] = {data[i - index], true};
    }
    size_t tmpi = _assembled_index;
    string tmps;
    while (tmpi < _actual_size && _buffer[tmpi].second) {
        tmps.push_back(_buffer[tmpi].first);
        _buffer[tmpi].second = 0;
        _buffer_size--;
        tmpi++;
    }
    _assembled_index = tmpi;
    _output.write(tmps);
    if (_assembled_index >= _actual_size) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _buffer_size; }

bool StreamReassembler::empty() const { return _buffer_size == 0; }
