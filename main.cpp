#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <vector>

#include "bencode/bencode_parser.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_torrent_file>\n";
    return 1;
  }
  std::filesystem::path file_path = argv[1];

  if (!std::filesystem::exists(file_path)) {
    std::cerr << "Error: File does not exist at path: " << file_path << "\n";
    return 1;
  }
  if (!std::filesystem::is_regular_file(file_path)) {
    std::cerr << "Error: Path is not a regular file.\n";
    return 1;
  }
  std::uintmax_t file_size = std::filesystem::file_size(file_path);

  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Failed to open file.\n";
    return 1;
  }

  std::vector<std::byte> file_buffer(file_size);

  file.read(reinterpret_cast<char*>(file_buffer.data()), file_size);
  std::span<const std::byte> file_span = file_buffer;

  try {
    MetaInfo torrent = parse_torrent(file_span);
    std::cout << "Successfully parsed! Tracker: " << torrent.info.length << "\n";
  } catch (const std::exception& e) {
    std::cerr << "Fatal Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}