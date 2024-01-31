#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    auto &th = seg.header();
    bool ret = false;
    if (th.syn) {
        if (_syn) return false;
        _isn = seg.header().seqno;
        _syn = true;
        _base = 1;
        ret = true;
    }
    if (!_syn) return false;
    auto t = unwrap(WrappingInt32(th.seqno), _isn, 0);
    if (th.fin) {
        if (_fin) return false;
        _fin = true;
        ret = true;
    }

    if (_base + window_size() - 1 < t || t + seg.payload().size() - 1 < _base) {
        if (!ret) return false;
    }
    _reassembler.push_substring(seg.payload().copy(),t - 1, th.fin);
    _base = _reassembler.assembled_num() + 1;
    if (_reassembler.stream_out().input_ended()) _base++;
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn) return {};
    return _isn + _reassembler.assembled_num() + ((_fin && _reassembler.empty()) ? 1 : 0) + 1;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
