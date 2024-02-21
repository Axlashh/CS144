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
    //syn_sent || listen
    if (!_syn && seg.header().rst) return false;
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
        if (seg.header().rst && t == _base) return true;
        if (!ret) return false;
    }
    _reassembler.push_substring(seg.payload().copy(),t - 1, th.fin);
    _base = _reassembler.assembled_num() + 1;
    if (_reassembler.stream_out().input_ended()) _base++;
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn) return {};
    return wrap(_base, _isn);
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }

// #include "tcp_receiver.hh"

// #include <optional>

// // Dummy implementation of a TCP receiver

// // For Lab 2, please replace with a real implementation that passes the
// // automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

// using namespace std;

// bool TCPReceiver::segment_received(const TCPSegment &seg) {
//     bool ret = false;
//     static size_t abs_seqno = 0;
//     size_t length;
//     if (seg.header().syn) {
//         if (_syn_flag) {  // already get a SYN, refuse other SYN.
//             return false;
//         }
//         _syn_flag = true;
//         ret = true;
//         _isn = seg.header().seqno.raw_value();
//         abs_seqno = 1;
//         _base = 1;
//         length = seg.length_in_sequence_space() - 1;
//         if (length == 0) {  // segment's content only have a SYN flag
//             return true;
//         }
//     } else if (!_syn_flag) {  // before get a SYN, refuse any segment
//         return false;
//     } else {  // not a SYN segment, compute it's abs_seqno
//         abs_seqno = unwrap(WrappingInt32(seg.header().seqno.raw_value()), WrappingInt32(_isn), abs_seqno);
//         length = seg.length_in_sequence_space();
//     }

//     if (seg.header().fin) {
//         if (_fin_flag) {  // already get a FIN, refuse other FIN
//             return false;
//         }
//         _fin_flag = true;
//         ret = true;
//     }
//     // not FIN and not one size's SYN, check border
//     else if (seg.length_in_sequence_space() == 0 && abs_seqno == _base) {
//         return true;
//     } else if (abs_seqno >= _base + window_size() || abs_seqno + length <= _base) {
//         if (!ret)
//             return false;
//     }

//     _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
//     _base = _reassembler.head_index() + 1;
//     if (_reassembler.input_ended())  // FIN be count as one byte
//         _base++;
//     return true;
// }

// optional<WrappingInt32> TCPReceiver::ackno() const {
//     if (_base > 0)
//         return WrappingInt32(wrap(_base, WrappingInt32(_isn)));
//     else
//         return std::nullopt;
// }

// size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }