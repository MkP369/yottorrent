#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <vector>

#include "file_io.hpp"
#include "generate_id.hpp"
#include "p2p_manager/master_manager.hpp"
#include "torrent_file/bencode_parser.hpp"
#include "tracker/tracker.hpp"

std::array<uint8_t, 20> local_id;

int main(int argc, char* argv[]) {
  std::span<char*> args(argv, static_cast<size_t>(argc));
  if (argc < 3) {
    std::cerr << "Usage: " << args[0]
              << " <path_to_torrent_file> <download_path>\n";
    return 1;
  }
  generate_id(local_id);

  try {
    std::vector<std::uint8_t> file_buffer = read_binary_file(args[1]);
    const MetaInfo torrent = parse_torrent_file(file_buffer);
    const TrackerResponse resp = request_peers(torrent, local_id, 6886);
    std::filesystem::path download_path = args[2];
    std::cout<<"Starting download\n";
    start_download(torrent.info, resp, download_path, local_id);
  } catch (const std::exception& e) {
    std::cerr << "Fatal Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
