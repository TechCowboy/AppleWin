#pragma once
#include "Response.h"
#include <array>

class ReadBlockResponse : public Response
{
public:
  explicit ReadBlockResponse(uint8_t request_sequence_number, uint8_t status);
  std::vector<uint8_t> serialize() const override;

  void set_block_data(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);
  const std::array<uint8_t, 512> &get_block_data() const;

private:
  std::array<uint8_t, 512> block_data_;
};
