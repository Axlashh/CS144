#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    auto win = _window_size > 0 ? _window_size : 1;
    auto _bytes_sents = win - (_next_seqno - _acked_seqno);
    if (_next_seqno == 0) {
        TCPSegment ts;
        ts.header().seqno = wrap(0, _isn);
        ts.header().syn = true;
        _segments_out.push(ts);
        _outstnding.push(ts);
        _next_seqno++;
        _bytes_in_flight++;
        _bytes_sents = win - (_next_seqno - _acked_seqno);
    }

    if (_stream.eof() && _next_seqno == _stream.bytes_written() + 2) 
        return;

    while ((_bytes_sents = win - (_next_seqno - _acked_seqno)) > 0) {
        auto _seg_len = min(_bytes_sents, TCPConfig::MAX_PAYLOAD_SIZE);
        TCPSegment ts;
        ts.payload() = Buffer(_stream.read(_seg_len));
        ts.header().seqno = wrap(_next_seqno, _isn);
        if (_stream.eof()) {
            ts.header().fin = true;
            _bytes_sents = 0;
        }
        _next_seqno += ts.length_in_sequence_space();
        _bytes_in_flight += ts.length_in_sequence_space();
        if (ts.length_in_sequence_space() == 0) break;
        _segments_out.push(ts);
        _outstnding.push(ts);
        if (ts.header().fin) break;
    }
}



//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size = window_size;
    auto abs_ackno = unwrap(ackno, _isn, 0);
    if (abs_ackno > _next_seqno) 
        return false;
    if (abs_ackno <= _acked_seqno)
        return true;
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    while (1) {
        if (_outstnding.empty()) break;
        auto olddest = _outstnding.front();
        if (unwrap(olddest.header().seqno, _isn, 0) + olddest.length_in_sequence_space() <= abs_ackno) {
           _outstnding.pop();
           _bytes_in_flight -= olddest.length_in_sequence_space();
           _acked_seqno = max(_acked_seqno, unwrap(olddest.header().seqno, _isn, 0) + olddest.length_in_sequence_space());
        } else {
            break;
        }
    }
    fill_window();
    if (!_outstnding.empty()) 
        _timer = 0;
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer += ms_since_last_tick;
    if (_timer >= _retransmission_timeout) {
        _segments_out.push(_outstnding.front());
        if (_window_size > 0) {
            _consecutive_retransmissions++;
            _retransmission_timeout *= 2;
        }
        _timer = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment sent;
    sent.header().seqno = wrap(_next_seqno, _isn);
    if (_next_seqno == 0) sent.header().syn = true;
    if (_stream.eof() && _next_seqno == _stream.bytes_written() + 2) sent.header().fin = true;
    _segments_out.push(sent);
}
