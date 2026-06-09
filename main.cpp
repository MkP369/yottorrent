#include <filesystem>
#include <iostream>

#include "file_io.hpp"
#include "torrent_file/bencode_parser.hpp"
#include "tracker/tracker.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_torrent_file>\n";
    return 1;
  }

  try {
    std::vector<std::uint8_t> file_buffer = read_binary_file(argv[1]);
    const MetaInfo torrent = parse_torrent_file(file_buffer);
    const std::array<uint8_t, 20> local_id = {'T', 'O', 'R', 'R', 'E', 'N', 'T',
                                              'T', 'O', 'R', 'R', 'E', 'N', 'T',
                                              '6', '9', '7', '2', '0', '3'};
    const TrackerResponse resp = request_peers(torrent, local_id, 6886);
    std::cout << resp.interval;
  } catch (const std::exception& e) {
    std::cerr << "Fatal Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
