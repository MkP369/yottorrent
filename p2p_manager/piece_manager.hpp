#pragma once
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

#include "block.hpp"
#include "p2p_manager/file_storage.hpp"
#include "peer/bitfield/bitfield.hpp"
#include "piece.hpp"
class PieceManager {
 public:
  PieceManager(uint32_t total_pieces, uint32_t piece_length,
               uint64_t total_size,
               const std::vector<std::array<uint8_t, 20>>& piece_hashes) {
    m_pieces.reserve(total_pieces);
    for (uint32_t i = 0; i < total_pieces - 1; i++) {
      m_pieces.emplace_back(i, piece_length, piece_hashes.at(i));
    }
    if (total_size % piece_length == 0) {
      m_pieces.emplace_back(total_pieces - 1, piece_length,
                            piece_hashes.at(total_pieces - 1));
    } else {
      m_pieces.emplace_back(total_pieces - 1, total_size % piece_length,
                            piece_hashes.at(total_pieces - 1));
    }
  }

  std::optional<Block> get_next_missing_block(
      BitField& peer_bitfield,
      const boost::asio::ip::tcp::endpoint& peer_endpoint) {
    for (auto& piece : m_pieces) {
      if (piece.is_partial() &&
          peer_bitfield.has_piece(piece.get_piece_index())) {
        auto [found, block] = piece.get_missing_block();
        if (found) {
          piece.update_block_state(block, BlockState::PENDING);
          track_pending_block(peer_endpoint, block);
          return block;
        }
      }
    }

    for (auto& piece : m_pieces) {
      if (!piece.is_partial() && !piece.is_completed() &&
          peer_bitfield.has_piece(piece.get_piece_index())) {
        auto [found, block] = piece.get_missing_block();
        if (found) {
          piece.update_block_state(block, BlockState::PENDING);
          track_pending_block(peer_endpoint, block);
          return block;
        }
      }
    }
    return std::nullopt;
  }

  void mark_block_completed(uint32_t piece_idx, uint32_t offset,
                            const boost::asio::ip::tcp::endpoint& peer_endpoint,
                            std::vector<uint8_t> buf, FileStorage& f) {
    m_pieces.at(piece_idx).update_piece_data(buf, offset);
    size_t idx = 0;
    for (auto& it : m_pending_blocks_per_peer[peer_endpoint]) {
      if (it.offset == offset && it.piece_index == piece_idx) {
        m_pending_blocks_per_peer[peer_endpoint].erase(
            m_pending_blocks_per_peer[peer_endpoint].begin() + idx);
        break;
      }
      idx++;
    }
    if (m_pieces.at(piece_idx).is_completed()) {
      m_pieces.at(piece_idx).check_and_write_piece(f);
      m_completed_pieces++;
      std::cout << "Progress: " << (1.0 * m_completed_pieces) / m_pieces.size()
                << "\n";
    };
  }

  void handle_peer_disconnect(
      const boost::asio::ip::tcp::endpoint& peer_endpoint) {
    for (auto& b : m_pending_blocks_per_peer[peer_endpoint]) {
      m_pieces.at(b.piece_index).revert_block(b);
    }
    m_pending_blocks_per_peer.erase(peer_endpoint);
  }
  [[nodiscard]] bool finished() const {
    return m_completed_pieces == m_pieces.size();
  }

 private:
  std::vector<Piece> m_pieces;
  uint32_t m_completed_pieces = 0;
  std::unordered_map<boost::asio::ip::tcp::endpoint, std::vector<Block>>
      m_pending_blocks_per_peer;
  void track_pending_block(const boost::asio::ip::tcp::endpoint& peer_endpoint,
                           const Block& block) {
    m_pending_blocks_per_peer[peer_endpoint].push_back(block);
  }
};
