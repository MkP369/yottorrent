#include <algorithm>
#include <array>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#include "bitfield/bitfield.hpp"
#include "handshake/handshake.hpp"
#include "message/message.hpp"
#include "p2p_manager/block.hpp"
#include "p2p_manager/file_storage.hpp"
#include "p2p_manager/piece_manager.hpp"

class PeerConnection {
 public:
  explicit PeerConnection(const boost::asio::any_io_executor& executor,
                          size_t total_pieces)
      : m_peer_socket(executor),
        m_peer_bitfield(total_pieces),
        m_total_pieces(total_pieces) {}

  // opens socket, connects to it, performs handshake & receives bitfield
  boost::asio::awaitable<void> new_connection(
      boost::asio::ip::tcp::endpoint peer_ep,
      const std::array<uint8_t, 20>& local_info_hash,
      const std::array<uint8_t, 20>& local_id) {
    std::cout << "Connecting with peer " << peer_ep << "\n";
    m_peer_socket.open(peer_ep.protocol());
    co_await m_peer_socket.async_connect(peer_ep, boost::asio::use_awaitable);
    co_await perform_handshake(m_peer_socket, local_info_hash, local_id);
    co_await receive_bitfield();
    m_peer_endpoint = peer_ep;
    std::cout << "Connected with peer " << peer_ep << "\n";
  }

  boost::asio::awaitable<void> send_request(uint32_t piece_index,
                                            uint32_t offset,
                                            uint32_t req_length) {
    msg request_msg = format_request(piece_index, offset, req_length);
    co_await send_msg(&request_msg);
  }
  boost::asio::awaitable<void> send_have(uint32_t piece_index) {
    msg have_msg = format_have(piece_index);
    co_await send_msg(&have_msg);
  }

  boost::asio::awaitable<void> send_interested() {
    msg interested_msg{.id = msg_id::INTERESTED, .payload = {}};
    co_await send_msg(&interested_msg);
  }

  boost::asio::awaitable<void> send_not_interested() {
    msg not_interested_msg{.id = msg_id::NOT_INTERESTED, .payload = {}};
    co_await send_msg(&not_interested_msg);
  }

  boost::asio::awaitable<void> send_choke() {
    msg unchoke_msg{.id = msg_id::CHOKE, .payload = {}};
    co_await send_msg(&unchoke_msg);
  }

  boost::asio::awaitable<void> send_unchoke() {
    msg unchoke_msg{.id = msg_id::UNCHOKE, .payload = {}};
    co_await send_msg(&unchoke_msg);
  }

  boost::asio::awaitable<void> send_cancel(uint32_t piece_index,
                                           uint32_t offset,
                                           uint32_t req_length) {
    msg cancel_msg = format_cancel(piece_index, offset, req_length);
    co_await send_msg(&cancel_msg);
  }
  boost::asio::awaitable<bool> read_msg(msg& message) {
    std::array<uint8_t, 4> length_buf{};
    co_await boost::asio::async_read(m_peer_socket,
                                     boost::asio::buffer(length_buf),
                                     boost::asio::use_awaitable);
    uint32_t length = (static_cast<uint32_t>(length_buf.at(0)) << 24) |
                      (static_cast<uint32_t>(length_buf.at(1)) << 16) |
                      (static_cast<uint32_t>(length_buf.at(2)) << 8) |
                      static_cast<uint32_t>(length_buf.at(3));
    if (length == 0) {
      co_return false;
    }

    std::vector<uint8_t> msg_type_payload_buf(length);
    co_await boost::asio::async_read(m_peer_socket,
                                     boost::asio::buffer(msg_type_payload_buf),
                                     boost::asio::use_awaitable);
    message.id = static_cast<msg_id>(msg_type_payload_buf.at(0));
    message.payload.resize(length - 1);
    if (length > 1) {
      std::copy(msg_type_payload_buf.begin() + 1, msg_type_payload_buf.end(),
                message.payload.begin());
    }
    co_return true;
  }

  boost::asio::awaitable<void> start_downloading(PieceManager& pm,
                                                 FileStorage& f) {
    uint8_t curr_backlog = 0;
    std::array<std::optional<Block>, 5> backlog;
    while (m_peer_socket.is_open() && !pm.finished()) {
      while (!am_choking && curr_backlog < backlog.size()) {
        if (!am_interested) {
          co_await send_interested();
          am_interested = true;
        }
        auto b = pm.get_next_missing_block(m_peer_bitfield, m_peer_endpoint);
        if (b.has_value()) {
          curr_backlog++;
          for (auto& b_l : backlog) {
            if (!b_l.has_value()) {
              b_l = b;
              break;
            }
          }
          co_await send_request(b->piece_index, b->offset, b->length);
        } else {
          break;
        }
      }
      msg m;
      bool is_not_keep_alive = co_await read_msg(m);
      if (is_not_keep_alive) {
        if (m.id == msg_id::PIECE) {
          std::vector<uint8_t> buf;
          auto [piece_idx, offset] = parse_piece(&m, buf);
          pm.mark_block_completed(piece_idx, offset, m_peer_endpoint, buf, f);
          bool flag_updated = false;
          for (auto& b : backlog) {
            if (b.has_value() && b.value().offset == offset &&
                b.value().piece_index == piece_idx &&
                buf.size() == b.value().length) {
              b.reset();
              curr_backlog--;
              flag_updated = true;
            }
          }
          if (!flag_updated) {
            throw std::runtime_error("received wrong block");
          }
        } else if (m.id == msg_id::HAVE) {
          m_peer_bitfield.set_piece(parse_have(&m));
        } else if (m.id == msg_id::UNCHOKE) {
          am_choking = false;
        } else if (m.id == msg_id::CHOKE) {
          am_choking = true;
        }
      }
    }
  }

 private:
  boost::asio::ip::tcp::socket m_peer_socket;
  boost::asio::ip::tcp::endpoint m_peer_endpoint;
  BitField m_peer_bitfield;
  size_t m_total_pieces;
  bool am_choking = true;
  bool am_interested = false;
  // bool peer_choking = true;
  // bool peer_interested = false;

  boost::asio::awaitable<void> send_msg(msg* message) {
    std::vector<uint8_t> out_buffer = serialise(message);
    co_await boost::asio::async_write(m_peer_socket,
                                      boost::asio::buffer(out_buffer),
                                      boost::asio::use_awaitable);
  }

  boost::asio::awaitable<void> receive_bitfield() {
    msg bitfield_message;
    bool is_payload_present = co_await read_msg(bitfield_message);
    if (!is_payload_present) {
      throw std::runtime_error(
          "expected message of type bitfield, got keep-alive packet");
    }

    if (bitfield_message.id != msg_id::BITFIELD) {
      throw std::runtime_error(
          "expected message of type bitfield after handshake");
    }

    if (bitfield_message.payload.size() != (m_total_pieces + 8 - 1) / 8) {
      throw std::runtime_error("bitfield payload is of wrong size");
    }
    // check spare bits are 0
    uint32_t last_piece_index = m_total_pieces % 8;
    if (last_piece_index != 0 &&
        ((bitfield_message.payload.back() << last_piece_index) & 0xFF) != 0) {
      throw std::runtime_error("bitfield spare bits are not zero");
    }
    m_peer_bitfield.update_bitfield(bitfield_message.payload);
  }
};
