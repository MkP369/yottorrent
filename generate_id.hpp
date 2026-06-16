#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <random>
#include <string>

inline void generate_id(std::array<uint8_t, 20>& local_id) {
  local_id = {'-', 'Y', 'T', '0', '0', '0', '1', '-'};
  const std::string charset =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(0, 61);
  std::generate(std::next(local_id.begin(), 8), local_id.end(),
                [&]() { return charset.at(static_cast<size_t>(distr(gen))); });
}
