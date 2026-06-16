#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

enum msg_id : uint8_t {
  CHOKE,
  UNCHOKE,
  INTERESTED,
  NOT_INTERESTED,
  HAVE,
  BITFIELD,
  REQUEST,
  PIECE,
  CANCEL
};

struct msg {
  msg_id id;
  std::vector<uint8_t> payload;
};

inline std::vector<uint8_t> serialise(const msg* data) {
  if (data == nullptr) {
    return std::vector<uint8_t>(4, 0);
  }
  std::vector<uint8_t> buffer(5 + data->payload.size());
  const uint32_t length = 1 + data->payload.size();
  buffer.at(0) = (length >> 24);
  buffer.at(1) = (length >> 16);
  buffer.at(2) = (length >> 8);
  buffer.at(3) = (length);
  buffer.at(4) = data->id;
  std::ranges::copy(data->payload.begin(), data->payload.end(),
                    buffer.begin() + 5);
  return buffer;
}
inline msg format_request(uint32_t piece_index, uint32_t offset,
                          uint32_t req_length) {
  msg req_msg;
  req_msg.id = msg_id::REQUEST;
  req_msg.payload.resize(12);

  // put piece index
  req_msg.payload.at(0) = piece_index >> 24;
  req_msg.payload.at(1) = piece_index >> 16;
  req_msg.payload.at(2) = piece_index >> 8;
  req_msg.payload.at(3) = piece_index;
  // put offset
  req_msg.payload.at(4) = offset >> 24;
  req_msg.payload.at(5) = offset >> 16;
  req_msg.payload.at(6) = offset >> 8;
  req_msg.payload.at(7) = offset;
  // put requested length
  req_msg.payload.at(8) = req_length >> 24;
  req_msg.payload.at(9) = req_length >> 16;
  req_msg.payload.at(10) = req_length >> 8;
  req_msg.payload.at(11) = req_length;

  return req_msg;
}

inline msg format_cancel(uint32_t piece_index, uint32_t offset,
                         uint32_t req_length) {
  msg cancel_msg = format_request(piece_index, offset, req_length);
  cancel_msg.id = msg_id::CANCEL;
  return cancel_msg;
}

inline msg format_have(uint32_t piece_index) {
  msg have_msg;
  have_msg.id = msg_id::HAVE;
  have_msg.payload.resize(4);
  have_msg.payload.at(0) = piece_index >> 24;
  have_msg.payload.at(1) = piece_index >> 16;
  have_msg.payload.at(2) = piece_index >> 8;
  have_msg.payload.at(3) = piece_index;
  return have_msg;
}

inline std::pair<uint32_t, uint32_t> parse_piece(msg* piece_msg,
                                                 std::vector<uint8_t>& buf) {
  if (piece_msg->id != msg_id::PIECE) {
    throw std::runtime_error("expected message of type 'PIECE'");
  }
  if (piece_msg->payload.size() < 8) {
    throw std::runtime_error("payload too small");
  }
  uint32_t parsed_piece_index =
      ((static_cast<uint32_t>(piece_msg->payload.at(0)) << 24) |
       (static_cast<uint32_t>(piece_msg->payload.at(1)) << 16) |
       (static_cast<uint32_t>(piece_msg->payload.at(2)) << 8) |
       static_cast<uint32_t>(piece_msg->payload.at(3)));
  uint32_t begin_index =
      ((static_cast<uint32_t>(piece_msg->payload.at(4)) << 24) |
       (static_cast<uint32_t>(piece_msg->payload.at(5)) << 16) |
       (static_cast<uint32_t>(piece_msg->payload.at(6)) << 8) |
       static_cast<uint32_t>(piece_msg->payload.at(7)));
  buf.assign(piece_msg->payload.begin() + 8, piece_msg->payload.end());
  return {parsed_piece_index, begin_index};
}

inline uint32_t parse_have(msg* have_msg) {
  if (have_msg->id != msg_id::HAVE) {
    throw std::runtime_error("expected message of type 'HAVE'");
  }
  if (have_msg->payload.size() != 4) {
    throw std::runtime_error(
        "message of type 'HAVE' should have payload of size 4");
  }
  return ((static_cast<uint32_t>(have_msg->payload.at(0)) << 24) |
          (static_cast<uint32_t>(have_msg->payload.at(1)) << 16) |
          (static_cast<uint32_t>(have_msg->payload.at(2)) << 8) |
          static_cast<uint32_t>(have_msg->payload.at(3)));
}
