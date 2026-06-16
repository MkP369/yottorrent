#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
class BitField {
 public:
  explicit BitField(size_t total_pieces)
      : m_bitfield((total_pieces + 8 - 1) / 8,
                   0),  // ceil(a/b) = (a+b-1)/b

        m_total_pieces(total_pieces) {}
  [[nodiscard]] bool has_piece(const size_t piece_index) const {
    if (piece_index < m_total_pieces) {
      const size_t chunk = piece_index / 8;
      const size_t offset = piece_index % 8;
      return (m_bitfield.at(chunk) & (1 << (7 - offset))) != 0;
    }
    throw std::runtime_error("given index does not exist in the bitfield");
  }
  void set_piece(const size_t piece_index) {
    if (piece_index < m_total_pieces) {
      const size_t chunk = piece_index / 8;
      const size_t offset = piece_index % 8;
      m_bitfield.at(chunk) |= (1 << (7 - offset));
    } else {
      throw std::runtime_error("given index does not exist in the bitfield");
    }
  }
  void update_bitfield(std::vector<uint8_t>& bf) {
    if (bf.size() != m_bitfield.size()) {
      throw std::runtime_error("size of bitfields do not match");
    }
    std::ranges::copy(bf, m_bitfield.begin());
  }

 private:
  std::vector<std::uint8_t> m_bitfield;
  size_t m_total_pieces;
};
