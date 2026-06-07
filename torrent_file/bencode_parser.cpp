#include "bencode_parser.hpp"

#include <boost/hash2/sha1.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class BencodeParser {
 public:
  explicit BencodeParser(std::span<const std::uint8_t> data)
      : m_data(data), m_index(0) {}
  MetaInfo parse_torrent_file() {
    if (peek() == 'd')
      consume(1);
    else
      throw std::runtime_error("expected d");
    MetaInfo m;
    bool has_announce = false, has_info = false;
    std::string prev_key = "";  // to check lexographical ordering of the keys
    while (peek() != 'e') {
      std::string key = parse_string();
      if (key <= prev_key) {
        throw std::runtime_error(
            "dictionary keys are not in lexicographic order");
      }
      prev_key = key;
      if (key == "announce") {
        m.announce = parse_string();
        has_announce = true;
      } else if (key == "announce-list") {
        m.announce_list = parse_announce_list();
      } else if (key == "comment") {
        m.comment = parse_string();
      } else if (key == "created by") {
        m.created_by = parse_string();
      } else if (key == "creation date") {
        m.creation_date = parse_int();
      } else if (key == "encoding") {
        m.encoding = parse_string();
      } else if (key == "info") {
        m.info = parse_torrent_info();
        has_info = true;
      } else {
        skip();
      }
    }
    if (!(has_announce && has_info)) {
      throw std::runtime_error(
          "mandatory fields in MetaInfo dictionary are missing");
    }
    consume(1);  // 'e'
    return m;
  }

  TrackerResponse parse_tracker_response() {
    TrackerResponse t;
    if (peek() == 'd')
      consume(1);
    else
      throw std::runtime_error("expected  d");
    bool has_interval = false, has_peers = false;
    std::string prev_key = "";
    while (peek() != 'e') {
      std::string key = parse_string();
      if (key <= prev_key) {
        throw std::runtime_error(
            "dictionary keys are not in lexicographic order");
      }
      prev_key = key;
      if (key == "interval") {
        t.interval = parse_int();
        has_interval = true;
      } else if (key == "peers") {
        if (peek() == 'l') {
          t.peers = parse_normal_peers();
        } else {  // compact form
          t.peers = parse_compact_peers();
        }
        has_peers = true;
      } else {
        skip();
      }
    }
    if (!(has_peers && has_interval)) {
      throw std::runtime_error(
          "mandatory fields in TrackerResponse dictionary are missing");
    }
    consume(1);  // 'e'
    return t;
  }

 private:
  std::span<const uint8_t> m_data;
  size_t m_index = 0;

  char peek() {
    if (m_index >= m_data.size())
      throw std::runtime_error("Can't peek, Unexpected EOF");

    return static_cast<char>(m_data[m_index]);
  }

  void consume(size_t length) {
    if (m_index + length > m_data.size())
      throw std::runtime_error("Can't consume, Unexpected EOF");
    else
      m_index += length;
  }

  std::vector<boost::asio::ip::tcp::endpoint> parse_compact_peers() {
    std::vector<boost::asio::ip::tcp::endpoint> peers;
    const std::string raw_peers = parse_string();
    const size_t peer_size = 6;  // 4 ip + 2 port
    if (raw_peers.size() % 6 != 0) {
      throw std::runtime_error("malformed peers value");
    }
    size_t total_peers = raw_peers.size() / peer_size;
    peers.resize(total_peers);
    boost::asio::ip::address_v4::bytes_type ip_bytes;
    uint16_t port;
    for (size_t i = 0; i < total_peers; i++) {
      std::copy_n(raw_peers.begin() + peer_size * i, 4, ip_bytes.begin());
      boost::asio::ip::address_v4 addr(ip_bytes);
      port = (static_cast<uint16_t>(raw_peers[peer_size * i + 4]) << 8) |
             raw_peers[peer_size * i + 5];
      peers[i] = boost::asio::ip::tcp::endpoint(addr, port);
    }
    return peers;
  }

  std::vector<boost::asio::ip::tcp::endpoint> parse_normal_peers() {
    std::vector<boost::asio::ip::tcp::endpoint> peers;
    if (peek() != 'l') {
      throw std::runtime_error("expected 'l'");
    }
    consume(1);
    while (peek() != 'e') {
      if (peek() != 'd') {
        throw std::runtime_error("expected 'd'");
      }
      consume(1);

      std::string key = parse_string();
      if (key != "ip") {
        throw std::runtime_error("expected key 'ip'");
      }
      const std::string ip_str = parse_string();

      key = parse_string();
      if (key != "peer id") {
        throw std::runtime_error("expected key 'peer id'");
      }
      skip();  // peer id is not required

      key = parse_string();
      if (key != "port") {
        throw std::runtime_error("expected key 'port'");
      }
      const long long port = parse_int();
      if (!(port >= 0 && port <= 65535)) {
        throw std::runtime_error("not a valid port");
      }
      peers.push_back(string_to_ip(ip_str, port));
      consume(1);
    }
    consume(1);
    return peers;
  }

  boost::asio::ip::tcp::endpoint string_to_ip(const std::string& ip_str,
                                              const uint16_t port) {
    if (ip_str.find(':') != std::string::npos) {  // IPv6
      return boost::asio::ip::tcp::endpoint(
          boost::asio::ip::make_address_v6(ip_str), port);
    } else if (std::any_of(ip_str.begin(), ip_str.end(), [](char c) {
                 return std::isalpha(c) || c == '-';
               })) {  // DNS name
      boost::asio::io_context io_context;
      boost::asio::ip::tcp::resolver resolver(io_context);
      auto endpoints = resolver.resolve(ip_str, std::to_string(port));
      for (const auto& entry : endpoints) {
        return entry.endpoint();
      }
    }
    // IPv4
    return boost::asio::ip::tcp::endpoint(
        boost::asio::ip::make_address_v4(ip_str), port);
  }
  long long parse_int() {
    char c = peek();
    if (c == 'i') {
      consume(1);  // 'i'
      std::string s;
      while (peek() != 'e') {
        s += peek();
        consume(1);
      }
      consume(1);  // 'e'
      long long num = std::stoll(s);
      if (s[0] == '+') {
        if (std::to_string(num).size() != s.size() - 1) {
          throw std::runtime_error(
              "Contains leading zeroes or a negative zero");
        }
      } else if (std::to_string(num).size() != s.size()) {
        throw std::runtime_error("Contains leading zeroes or a negative zero");
      }
      return num;
    } else
      throw std::runtime_error("expected 'i'");
  }

  std::string parse_string() {
    std::string s;
    while (peek() != ':') {
      s += peek();
      consume(1);
    }
    consume(1);  // ':'
    const long long len = std::stoll(s);
    if (len < 0) {
      throw std::runtime_error("string cannot have negative length");
    } else if (s[0] == '+') {
      if (std::to_string(len).size() != s.size() - 1) {
        throw std::runtime_error("Contains leading zeroes or a negative zero");
      }
    } else if (std::to_string(len).size() != s.size()) {
      throw std::runtime_error("Contains leading zeroes or a negative zero");
    }
    std::string content;
    content.reserve(len);
    for (int i = 0; i < len; i++) {
      content += peek();
      consume(1);
    }
    return content;
  }

  std::vector<std::array<uint8_t, 20>> parse_pieces() {
    std::string s;
    while (peek() != ':') {
      s += peek();
      consume(1);
    }
    consume(1);  // ':'
    const long long len = std::stoll(s);
    if (len < 0) {
      throw std::runtime_error("length of piece cannot be negative");
    } else if (s[0] == '+') {
      if (std::to_string(len).size() != s.size() - 1) {
        throw std::runtime_error(
            "length of piece contains leading zeroes or a negative zero");
      }
    } else if (std::to_string(len).size() != s.size()) {
      throw std::runtime_error(
          "length of piece contains leading zeroes or a negative zero");
    } else if (len % 20 != 0) {
      throw std::runtime_error("length of piece should be a multiple of 20");
    }

    const size_t total_bytes = static_cast<size_t>(len);  // for performance
    const size_t num_pieces = total_bytes / 20;
    std::vector<std::array<uint8_t, 20>> pieces(num_pieces);

    for (size_t chunk = 0; chunk < num_pieces; chunk++) {
      for (size_t byte_index = 0; byte_index < 20; byte_index++) {
        pieces[chunk][byte_index] = static_cast<uint8_t>(peek());
        consume(1);
      }
    }
    return pieces;
  }

  std::vector<std::vector<std::string>> parse_announce_list() {
    if (peek() == 'l')
      consume(1);
    else
      throw std::runtime_error("expected l");

    std::vector<std::vector<std::string>> val;
    while (peek() != 'e') {
      if (peek() == 'l')
        consume(1);
      else
        throw std::runtime_error("expected l");

      std::vector<std::string> l;
      while (peek() != 'e') {
        l.push_back(parse_string());
      }
      val.push_back(l);
      consume(1);  // 'e'
    }
    consume(1);  // 'e'
    return val;
  }

  TorrentInfo parse_torrent_info() {
    boost::hash2::sha1_160 hasher;
    const size_t dict_start = m_index;
    if (peek() == 'd')
      consume(1);
    else
      throw std::runtime_error("expected d");
    TorrentInfo t;
    std::string prev_key = "";
    bool has_length = false, has_name = false, has_piece_length = false,
         has_pieces = false;
    while (peek() != 'e') {
      std::string key = parse_string();
      if (key <= prev_key) {
        throw std::runtime_error("keys are not in lexicographic order");
      }
      prev_key = key;
      if (key == "length") {
        long long val = parse_int();
        if (val <= 0) {
          throw std::runtime_error("length should be greater than 0");
        }
        t.length = val;
        has_length = true;
      } else if (key == "md5sum") {
        std::string val = parse_string();
        if (val.size() != 32) {
          throw std::runtime_error("md5sum should be exactly 32 characters");
        }
        t.md5sum = val;
      } else if (key == "name") {
        t.name = parse_string();
        has_name = true;
      } else if (key == "piece length") {
        long long val = parse_int();
        if (val <= 0) {
          throw std::runtime_error("piece length should be greater than 0");
        }
        t.piece_length = val;
        has_piece_length = true;
      } else if (key == "pieces") {
        t.pieces = parse_pieces();
        has_pieces = true;
      } else {
        skip();
      }
    }
    if (!(has_length && has_name && has_pieces && has_piece_length))
      throw std::runtime_error("mandatory fields missing in TorrentInfo");
    consume(1);  // 'e'
    const size_t dict_end = m_index;
    hasher.update(m_data.data() + dict_start, dict_end - dict_start);
    auto digest = hasher.result();
    std::copy(digest.begin(), digest.end(), t.info_hash.begin());
    return t;
  }

  void skip() {
    if (peek() == 'i') {
      parse_int();
    } else if (peek() == 'd') {
      consume(1);
      std::string prev = "";
      while (peek() != 'e') {
        std::string curr = parse_string();
        if (curr <= prev) {
          throw std::runtime_error("keys are not lexicographically ordered");
        }
        prev = curr;
        skip();
      }
      consume(1);
    } else if (peek() == 'l') {
      consume(1);
      while (peek() != 'e') {
        skip();
      }
      consume(1);
    } else {
      parse_string();
    }
  }
};

MetaInfo parse_torrent_file(std::span<const uint8_t> data) {
  BencodeParser parser(data);
  return parser.parse_torrent_file();
}

TrackerResponse parse_tracker_response(std::span<const uint8_t> data) {
  BencodeParser parser(data);
  return parser.parse_tracker_response();
}
