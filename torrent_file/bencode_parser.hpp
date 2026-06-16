#pragma once
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct TorrentInfo {
  std::optional<std::string> md5sum;
  std::string name;
  size_t piece_length;
  std::vector<std::array<uint8_t, 20>> piece_hashes;
  size_t length;
  std::array<uint8_t, 20> info_hash;
};
struct MetaInfo {
  std::string announce;
  std::optional<std::vector<std::vector<std::string>>> announce_list;
  std::optional<std::string> comment;
  std::optional<std::string> created_by;
  std::optional<size_t> creation_date;
  std::optional<std::string> encoding;
  TorrentInfo info;
};

struct TrackerResponse {
  size_t interval;
  std::vector<boost::asio::ip::tcp::endpoint> peers;
};

MetaInfo parse_torrent_file(std::span<const uint8_t> data);

TrackerResponse parse_tracker_response(std::span<const uint8_t> data);
