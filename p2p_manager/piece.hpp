#pragma once
#include <algorithm>
#include <array>
#include <boost/hash2/sha1.hpp>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "block.hpp"
#include "p2p_manager/file_storage.hpp"
const uint32_t BLOCK_SIZE = 16384;
class Piece {
 public:
  Piece(uint32_t piece_index, uint32_t piece_length,
        const std::array<uint8_t, 20>& piece_hash)
      : m_piece_index(piece_index),
        m_piece_length(piece_length),
        m_piece_hash(piece_hash) {
    m_blocks.resize((piece_length + BLOCK_SIZE - 1) / BLOCK_SIZE);
    uint32_t curr_piece_length = piece_length;
    uint32_t curr_offset = 0;
    for (auto& b : m_blocks) {
      b.piece_index = piece_index;
      b.length = std::min(curr_piece_length, BLOCK_SIZE);
      b.offset = curr_offset;
      curr_piece_length -= BLOCK_SIZE;
      curr_offset += BLOCK_SIZE;
    }
  }
  [[nodiscard]] bool is_partial() const {
    if (is_completed()) {
      return false;
    }
    return std::ranges::any_of(
        m_blocks, [](const auto& b) { return b.state != BlockState::MISSING; });
  }
  [[nodiscard]] bool is_completed() const {
    return std::ranges::all_of(m_blocks, [](const auto& b) {
      return b.state == BlockState::COMPLETED;
    });
  }
  [[nodiscard]] std::pair<bool, Block> get_missing_block() const {
    for (const Block& b : m_blocks) {
      if (b.state == BlockState::MISSING) {
        return {true, b};
      }
    }
    return {false, {}};
  }
  bool update_block_state(uint32_t offset, BlockState updated_state) {
    for (auto& local_b : m_blocks) {
      if (local_b.offset == offset) {
        local_b.state = updated_state;
        return true;
      }
    }
    return false;
  }
  bool update_block_state(Block& b, BlockState updated_state) {
    for (auto& local_b : m_blocks) {
      if (local_b.offset == b.offset && local_b.piece_index == b.piece_index &&
          local_b.length == b.length && local_b.state == b.state) {
        local_b.state = updated_state;
        return true;
      }
    }
    return false;
  }
  [[nodiscard]] uint32_t get_piece_index() const { return m_piece_index; }

  void update_piece_data(std::vector<uint8_t>& block_data, uint32_t offset) {
    if (!update_block_state(offset, BlockState::COMPLETED)) {
      throw std::runtime_error("given offset does not exist");
    }
    if (m_piece_data.contains(offset)) {
      throw std::runtime_error("data for this offset already exists");
    }
    m_piece_data[offset].assign(block_data.begin(), block_data.end());
  }

  void revert_piece() {
    for (auto& b : m_blocks) {
      b.state = BlockState::MISSING;
    }
    m_piece_data.clear();
  }
  void revert_block(Block& b) {
    m_piece_data.erase(b.offset);
    update_block_state(b.offset, BlockState::MISSING);
  }

  void check_and_write_piece(FileStorage& f) {
    std::vector<uint8_t> data(m_piece_length);
    for (auto& b : m_blocks) {
      if (!m_piece_data.contains(b.offset) ||
          !(b.length == m_piece_data[b.offset].size()) ||
          !(b.offset + b.length <= m_piece_length)) {
        revert_piece();
        throw std::runtime_error("malformed piece");
      }

      std::copy(m_piece_data[b.offset].begin(), m_piece_data[b.offset].end(),
                data.begin() + b.offset);
    }
    boost::hash2::sha1_160 hasher;
    hasher.update(data.data(), m_piece_length);
    auto digest = hasher.result();
    for (uint8_t i = 0; i < 20; i++) {
      if (digest[i] != m_piece_hash[i]) {
        revert_piece();
        throw std::runtime_error("hashes do not match");
      }
    }
    f.write_piece(m_piece_index, data);
    m_piece_data.clear();
  }

 private:
  uint32_t m_piece_index;
  uint32_t m_piece_length;
  std::vector<Block> m_blocks;
  std::unordered_map<uint32_t, std::vector<uint8_t>> m_piece_data;
  std::array<uint8_t, 20> m_piece_hash;
};
