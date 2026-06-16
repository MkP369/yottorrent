#include "handshake.hpp"

#include <algorithm>
#include <array>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <cstdint>
#include <stdexcept>

#pragma pack(push, 1)
struct HandShakePayload {
  std::uint8_t len_pstr = 19;
  std::array<uint8_t, 19> pstr = {'B', 'i', 't', 'T', 'o', 'r', 'r',
                                  'e', 'n', 't', ' ', 'p', 'r', 'o',
                                  't', 'o', 'c', 'o', 'l'};
  std::array<uint8_t, 8> reserved_bytes = {0, 0, 0, 0, 0, 0, 0, 0};
  std::array<uint8_t, 20> info_hash{};
  std::array<uint8_t, 20> peer_id{};
};
#pragma pack(pop)

boost::asio::awaitable<void> perform_handshake(
    boost::asio::ip::tcp::socket& peer_socket,
    const std::array<uint8_t, 20>& local_info_hash,
    const std::array<uint8_t, 20>& local_id) {
  HandShakePayload m_local_payload;
  std::copy_n(local_info_hash.begin(), 20, m_local_payload.info_hash.begin());
  std::copy_n(local_id.begin(), 20, m_local_payload.peer_id.begin());
  co_await boost::asio::async_write(peer_socket,
                                    boost::asio::buffer(&m_local_payload, 68),
                                    boost::asio::use_awaitable);

  HandShakePayload peer_payload;
  co_await boost::asio::async_read(peer_socket,
                                   boost::asio::buffer(&peer_payload, 68),
                                   boost::asio::use_awaitable);

  if (peer_payload.len_pstr != m_local_payload.len_pstr ||
      !(peer_payload.pstr == m_local_payload.pstr) ||
      !(peer_payload.info_hash == m_local_payload.info_hash)) {
    throw std::runtime_error("Malicious peer");
  }
}
