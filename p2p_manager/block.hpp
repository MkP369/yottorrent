#pragma once
#include <cstdint>

enum class BlockState : uint8_t { MISSING, PENDING, COMPLETED };
class Block {
 public:
  uint32_t piece_index{};
  uint32_t offset{};
  uint32_t length{};
  BlockState state = BlockState::MISSING;
};
