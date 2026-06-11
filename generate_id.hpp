#pragma once
#include <array>
#include <cstdint>
#include <random>

inline void generate_id(std::array<uint8_t, 20>& local_id) {
  local_id = {'-', 'Y', 'T', '0', '0', '0', '1', '-'};
  const std::string charset =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(0, 61);
  for (int i = 8; i < 20; i++) {
    local_id[i] = charset[distr(gen)];
  }
}
