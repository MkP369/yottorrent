#include "tracker.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/parameters.h"
#include "cpr/response.h"
#include "cpr/timeout.h"
#include "torrent_file/bencode_parser.hpp"

TrackerResponse request_peers(const MetaInfo& m,
                              const std::array<uint8_t, 20>& peer_id,
                              const uint16_t port) {
  std::string announce_url = m.announce;
  if (m.announce.at(0) == 'u') {
    if (m.announce_list.has_value()) {
      for (const auto& v : m.announce_list.value()) {
        for (auto url : v) {
          if (url.at(0) == 'h') {
            announce_url = url;
            break;
          }
        }
      }
    }
  }
  std::cout << announce_url << "\n";
  cpr::Response resp = cpr::Get(
      cpr::Url{announce_url},
      cpr::Parameters{{"info_hash", std::string(m.info.info_hash.begin(),
                                                m.info.info_hash.end())},
                      {"peer_id", std::string(peer_id.begin(), peer_id.end())},
                      {"port", std::to_string(port)},
                      {"uploaded", "0"},
                      {"downloaded", "0"},
                      {"compact", "1"},
                      {"left", std::to_string(m.info.length)}},
      cpr::Timeout(15000)  // 15 sec
  );

  if (resp.error) {
    throw std::runtime_error(resp.error.message);
  }
  return parse_tracker_response(std::span<uint8_t>(
      reinterpret_cast<uint8_t*>(resp.text.data()), resp.text.size()));
}
