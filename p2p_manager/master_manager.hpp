#pragma once
#include <array>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#include "file_storage.hpp"
#include "p2p_manager/piece_manager.hpp"
#include "peer/peer_connection.hpp"
#include "torrent_file/bencode_parser.hpp"

inline void start_download(const TorrentInfo& torrent,
                           const TrackerResponse& resp,
                           const std::filesystem::path& download_path,
                           std::array<uint8_t, 20> local_id) {
  boost::asio::io_context io_context;
  FileStorage f(download_path, torrent.length, torrent.piece_length);
  PieceManager pm(torrent.piece_hashes.size(), torrent.piece_length,
                  torrent.length, torrent.piece_hashes);
  std::cout << resp.peers.size() << "\n";
  std::vector<std::shared_ptr<PeerConnection>> peers;
  for (const auto& endpoint : resp.peers) {
    auto peer = std::make_shared<PeerConnection>(io_context.get_executor(),
                                                 torrent.piece_hashes.size());
    peers.push_back(peer);
    boost::asio::co_spawn(
        io_context.get_executor(),
        [peer, endpoint, &pm, &f, &local_id,
         &torrent]() -> boost::asio::awaitable<void> {
          try {
            co_await peer->new_connection(endpoint, torrent.info_hash,
                                          local_id);
            co_await peer->start_downloading(pm, f);
          } catch (...) {
            pm.handle_peer_disconnect(endpoint);
          }
        },
        boost::asio::detached);
  }
  io_context.run();
}
