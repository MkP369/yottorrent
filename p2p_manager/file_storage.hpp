#pragma once
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

class FileStorage {
 public:
  FileStorage(const std::string& path, size_t total_size, uint32_t piece_length)
      : m_piece_length(piece_length) {
    m_file.open(path, std::ios::out | std::ios::binary);
    m_file.seekp(total_size - 1);
    m_file.write("", 1);
    m_file.close();
    m_file.open(path, std::ios::in | std::ios::out | std::ios::binary);
  }

  ~FileStorage() {
    if (m_file.is_open()) {
      m_file.close();
    }
  }
  void write_piece(uint32_t piece_index, const std::vector<uint8_t>& data) {
    int64_t offset = m_piece_length * piece_index;
    m_file.seekp(offset);
    m_file.write(reinterpret_cast<const char*>(data.data()), data.size());
    m_file.flush();
  }

 private:
  std::fstream m_file;
  uint32_t m_piece_length;
};
