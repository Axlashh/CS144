#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    this->capa = capacity;
}

size_t ByteStream::write(const string &data) {
    if (!can_write) return 0;
    int able_write_num = capa > bytes.size() + data.size() ? data.size() : capa - bytes.size();
    for (int i = 0; i < able_write_num; i++) {
        bytes.push_back(data[i]);
    }
    write_num += able_write_num;
    return able_write_num;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    int lk_num = len > bytes.size() ? bytes.size() : len;
    string tmp;
    tmp.assign(bytes.begin(), bytes.begin() + lk_num);
    return tmp;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    int pop_num = len > bytes.size() ? bytes.size() : len;
    read_num += pop_num;
    bytes.erase(bytes.begin(), bytes.begin() + pop_num);
}

void ByteStream::end_input() {
    can_write = false;
}

bool ByteStream::input_ended() const { return !can_write;}

size_t ByteStream::buffer_size() const { return bytes.size(); }

bool ByteStream::buffer_empty() const { return bytes.size() == 0; }

bool ByteStream::eof() const { return bytes.size() == 0 && !can_write; }

size_t ByteStream::bytes_written() const { return write_num; }

size_t ByteStream::bytes_read() const { return read_num; }

size_t ByteStream::remaining_capacity() const { return capa - bytes.size(); }
