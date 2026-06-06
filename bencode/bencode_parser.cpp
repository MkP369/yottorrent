#include "bencode_parser.hpp"

#include <boost/hash2/sha1.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class BencodeParser {
 public:
  explicit BencodeParser(std::span<const std::byte> data)
      : m_data(data), m_index(0) {}
  MetaInfo parse() {
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
        throw std::runtime_error("unexpected key");
      }
    }
    if (!(has_announce && has_info)) {
      throw std::runtime_error(
          "mandatory fields in MetaInfo dictionary are missing");
    }
    // consume(1);  // 'e'
    return m;
  }

 private:
  std::span<const std::byte> m_data;
  size_t m_index = 0;

  char peek() {
    if (m_index >= m_data.size())
      throw std::runtime_error("Can't peek, Unexpected EOF");

    return static_cast<char>(m_data[m_index]);
  }

  void consume(int length) {
    if (m_index + length >= m_data.size())
      throw std::runtime_error("Can't consume, Unexpected EOF");
    else
      m_index += length;
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
    long long len = std::stoll(s);
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
    std::vector<std::array<uint8_t, 20>> pieces(len / 20);
    for (long long i = 0; i < len; i++) {
      long long chunk = i / 20;
      int byte_index = i % 20;
      pieces[chunk][byte_index] = static_cast<uint8_t>(peek());
      consume(1);
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

MetaInfo parse_torrent(std::span<const std::byte> data) {
  BencodeParser parser(data);
  return parser.parse();
}
