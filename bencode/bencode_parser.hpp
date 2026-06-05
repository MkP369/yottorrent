#pragma once
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct TorrentInfo {
  std::optional<std::string> md5sum;
  std::string name;
  long long piece_length;
  std::vector<std::byte> pieces;
  long long length;
};
struct MetaInfo {
  std::string announce;
  std::optional<std::vector<std::vector<std::string>>> announce_list;
  std::optional<std::string> comment;
  std::optional<std::string> created_by;
  std::optional<long long> creation_date;
  std::optional<std::string> encoding;
  TorrentInfo info;
};

MetaInfo parse_torrent(std::span<const std::byte> data);
