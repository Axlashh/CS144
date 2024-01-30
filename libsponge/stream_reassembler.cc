#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _eof(false), _buffer(), _assembled_index(0), _actual_size(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index + data.size() >= _capacity) {
        _eof = true;
    }
    if (eof && !_eof) {
        _eof = true;
        _actual_size = index + data.size();
    }
    if (_eof && index + data.size() >= _actual_size){
        for (size_t i = index; i < _actual_size; i++) {
            _buffer[i] = data[i - index];
        }
    } else {
        for (size_t i = index; i < index + data.size(); i++) {
            _buffer[i] = data[i - index];
        }
    }
    if (index <= _assembled_index) {
        int tmpi = _assembled_index;
        string tmps;
        while (_buffer.find(tmpi) != _buffer.end()) {
            tmps += _buffer[tmpi];
            _buffer.erase(tmpi);
            tmpi++;
        }
        _assembled_index = tmpi;
        _output.write(tmps);
        if (_eof && _assembled_index + 1 >= _actual_size) {
            _output.end_input();
        }
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _buffer.size(); }

bool StreamReassembler::empty() const { return _buffer.size() == 0; }
