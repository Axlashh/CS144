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
        //established
        if (!_listening && _receiver.segment_received(seg))
            unclean_shutdown();
        //syn_sent with good ack
        if (!_listening && !_receiver.ackno().has_value() && seg.header().ack && !unwrap(_sender.next_seqno(), seg.header().ackno, 0))
            unclean_shutdown();
        return;
    }
    //syn_sent with bad ack
    if (syn_sent() && unwrap(_sender.next_seqno(), seg.header().ackno, 0)) {
        // unclean_shutdown();
        // TCPSegment ts;
        // ts.header().rst = true;
        // ts.header().seqno = seg.header().ackno;
        // _segments_out.push(ts);
        return;
    }
    if (!_receiver.ackno().has_value() && seg.payload().size() != 0) {
        return;
    }
    if (_listening && !seg.header().syn) {
        //unclean_shutdown();
        return;
    }
    _receiver.segment_received(seg);
    bool send_emp = false;
    if (seg.payload().size() != 0) {
        send_emp = true;
    }
    if (seg.header().syn) {
        _listening = false;
        _isn = seg.header().seqno;
    }
    if (_listening)
        return;
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        if (unwrap(seg.header().ackno, _isn, 0) > _sender.next_seqno_absolute()) {
            send_emp = true;
        }
    }
    if (send_emp || seg.header().syn || seg.header().fin) {
        _sender.send_empty_segment();
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
    _time_last += ms_since_last_tick;
    try_clean_shutdown();
    _sender.tick(ms_since_last_tick);
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
    send_out();
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
    _listening = false;
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

void TCPConnection::unclean_shutdown(bool send_rst) {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _active = false;
    _send_rst = true;
    _listening = true;
    // if (_sender.segments_out().empty()) {
    //     _sender.send_empty_segment();
    // }
    if (send_rst) {
        send_out();
    }
}

bool TCPConnection::syn_sent() {
    return _sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute();
}

// #include "tcp_connection.hh"

// #include <iostream>

// // Dummy implementation of a TCP connection

// // For Lab 4, please replace with a real implementation that passes the
// // automated checks run by `make check`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

// using namespace std;

// size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

// size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

// size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

// size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

// void TCPConnection::segment_received(const TCPSegment &seg) {
//     if (!_active)
//         return;
//     _time_since_last_segment_received = 0;

//     // data segments with acceptable ACKs should be ignored in SYN_SENT
//     if (in_syn_sent() && seg.header().ack && seg.payload().size() > 0) {
//         return;
//     }
//     bool send_empty = false;
//     if (_sender.next_seqno_absolute() > 0 && seg.header().ack) {
//         // unacceptable ACKs should produced a segment that existed
//         if (!_sender.ack_received(seg.header().ackno, seg.header().win)) {
//             send_empty = true;
//         }
//     }

//     bool recv_flag = _receiver.segment_received(seg);
//     if (!recv_flag) {
//         send_empty = true;
//     }

//     if (seg.header().syn && _sender.next_seqno_absolute() == 0) {
//         connect();
//         return;
//     }

//     if (seg.header().rst) {
//         // RST segments without ACKs should be ignored in SYN_SENT
//         if (in_syn_sent() && !seg.header().ack) {
//             return;
//         }
//         unclean_shutdown(false);
//         return;
//     }

//     if (seg.length_in_sequence_space() > 0) {
//         send_empty = true;
//     }

//     if (send_empty) {
//         if (_receiver.ackno().has_value() && _sender.segments_out().empty()) {
//             _sender.send_empty_segment();
//         }
//     }
//     push_segments_out();
// }

// bool TCPConnection::active() const { return _active; }

// size_t TCPConnection::write(const string &data) {
//     size_t ret = _sender.stream_in().write(data);
//     push_segments_out();
//     return ret;
// }

// //! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
// void TCPConnection::tick(const size_t ms_since_last_tick) {
//     if (!_active)
//         return;
//     _time_since_last_segment_received += ms_since_last_tick;
//     _sender.tick(ms_since_last_tick);
//     if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
//         unclean_shutdown(true);
//     }
//     push_segments_out();
// }

// void TCPConnection::end_input_stream() {
//     _sender.stream_in().end_input();
//     push_segments_out();
// }

// void TCPConnection::connect() {
//     // when connect, must active send a SYN
//     push_segments_out(true);
// }

// TCPConnection::~TCPConnection() {
//     try {
//         if (active()) {
//             // Your code here: need to send a RST segment to the peer
//             cerr << "Warning: Unclean shutdown of TCPConnection\n";
//             unclean_shutdown(true);
//         }
//     } catch (const exception &e) {
//         std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
//     }
// }

// bool TCPConnection::push_segments_out(bool send_syn) {
//     // default not send syn before recv a SYN
//     _sender.fill_window(send_syn || in_syn_recv());
//     TCPSegment seg;
//     while (!_sender.segments_out().empty()) {
//         seg = _sender.segments_out().front();
//         _sender.segments_out().pop();
//         if (_receiver.ackno().has_value()) {
//             seg.header().ack = true;
//             seg.header().ackno = _receiver.ackno().value();
//             seg.header().win = _receiver.window_size();
//         }
//         if (_need_send_rst) {
//             _need_send_rst = false;
//             seg.header().rst = true;
//         }
//         _segments_out.push(seg);
//     }
//     clean_shutdown();
//     return true;
// }

// void TCPConnection::unclean_shutdown(bool send_rst) {
//     _receiver.stream_out().set_error();
//     _sender.stream_in().set_error();
//     _active = false;
//     if (send_rst) {
//         _need_send_rst = true;
//         if (_sender.segments_out().empty()) {
//             _sender.send_empty_segment();
//         }
//         push_segments_out();
//     }
// }

// bool TCPConnection::clean_shutdown() {
//     if (_receiver.stream_out().input_ended() && !(_sender.stream_in().eof())) {
//         _linger_after_streams_finish = false;
//     }
//     if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
//         if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
//             _active = false;
//         }
//     }
//     return !_active;
// }

// bool TCPConnection::in_listen() { return !_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0; }

// bool TCPConnection::in_syn_recv() { return _receiver.ackno().has_value() && !_receiver.stream_out().input_ended(); }

// bool TCPConnection::in_syn_sent() {
//     return _sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute();
// }