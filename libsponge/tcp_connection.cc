#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_last; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_last = 0;
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _rst = true;
        return;
    }
    _receiver.segment_received(seg);
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    send_out();
}

bool TCPConnection::active() const {
    
    return _active;
}

size_t TCPConnection::write(const string &data) {
    return _sender.stream_in().write(data);
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick);
    _time_last += ms_since_last_tick;
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        auto ts = _sender.segments_out().front();
        _sender.segments_out().pop();
        ts.header().rst = true;
        _segments_out.push(ts);
        return;
    }
    send_out();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    send_out();
}

void TCPConnection::connect() {
    _sender.fill_window();
    while (!_sender.segments_out().empty()) {
        _segments_out.push(_sender.segments_out().front());
        _sender.segments_out().pop();
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            unclean_shutdown();
            return;
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::send_out() {
    _sender.fill_window();
    if (_sender.segments_out().empty() && !(_sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0))
        _sender.send_empty_segment();
    while (!_sender.segments_out().empty()) {
        auto ts = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            ts.header().ack = true;
            ts.header().ackno = _receiver.ackno().value();
            ts.header().win = _receiver.window_size();
        }
        ts.header().rst = _send_rst;
        _segments_out.push(ts);
    }
    try_clean_shutdown();
}

bool TCPConnection::try_clean_shutdown() {
    if (_receiver.stream_out().input_ended() && !(_sender.stream_in().eof())) {
        _linger_after_streams_finish = false;
    }

    if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0) {
        if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            _active = false; 
        }
    }
    return !_active;
}

void TCPConnection::unclean_shutdown() {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _active = false;
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    send_out();
}