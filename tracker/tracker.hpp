#include "torrent_file/bencode_parser.hpp"
#include <array>
#include <cstdint>

TrackerResponse request_peers(const MetaInfo& m,
                              const std::array<uint8_t, 20>& peer_id,
                               uint16_t port);
