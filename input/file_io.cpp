#include "file_io.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

std::vector<std::uint8_t> read_binary_file(
    const std::filesystem::path& file_path) {
  if (!std::filesystem::exists(file_path)) {
    throw std::runtime_error("File does not exist: " + file_path.string());
  }
  if (!std::filesystem::is_regular_file(file_path)) {
    throw std::runtime_error("Path is not a regular file: " +
                             file_path.string());
  }

  std::uintmax_t file_size = std::filesystem::file_size(file_path);
  std::ifstream file(file_path, std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + file_path.string());
  }

  std::vector<std::uint8_t> file_buffer(file_size);
  file.read(reinterpret_cast<char*>(file_buffer.data()), file_size);
  if (!file) {
    throw std::runtime_error("Failed to read the entire file into memory.");
  }

  return file_buffer;
}
