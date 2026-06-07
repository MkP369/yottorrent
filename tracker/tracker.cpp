#include "tracker.hpp"

#include <cpr/cpr.h>

#include <stdexcept>
#include <string>

TrackerResponse request_peers(const MetaInfo& m,
                              const std::array<uint8_t, 20>& peer_id,
                              const uint16_t port) {
  cpr::Response resp = cpr::Get(
      cpr::Url{m.announce},
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
