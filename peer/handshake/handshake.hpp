#pragma once

#include <array>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdint>

boost::asio::awaitable<void> perform_handshake(
    boost::asio::ip::tcp::socket& peer_socket,
    const std::array<uint8_t, 20>& local_info_hash,
    const std::array<uint8_t, 20>& local_id);
