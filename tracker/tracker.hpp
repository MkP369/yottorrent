#include "torrent_file/bencode_parser.hpp"

TrackerResponse request_peers(const MetaInfo& m,
                              const std::array<uint8_t, 20>& peer_id,
                              const uint16_t port);
