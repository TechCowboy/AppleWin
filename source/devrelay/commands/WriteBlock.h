#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "../types/Request.h"
#include "../types/Response.h"

class WriteBlockRequest : public Request
{
public:
	WriteBlockRequest(uint8_t request_sequence_number, uint8_t device_id, uint16_t block_size);
	std::vector<uint8_t> serialize() const override;
	std::unique_ptr<Response> deserialize(const std::vector<uint8_t> &data) const override;
	const std::array<uint8_t, 3> &get_block_number() const;
	void set_block_number_from_ptr(const uint8_t *ptr, size_t offset);

	const std::vector<uint8_t> &get_block_data() const;
	void set_block_data_from_ptr(const uint8_t *ptr, size_t offset);
	void set_block_number_from_bytes(uint8_t l, uint8_t m, uint8_t h);
	const uint16_t get_block_size() const;

private:
	std::array<uint8_t, 3> block_number_;
	std::vector<uint8_t> block_data_;
	uint16_t block_size_;
};

class WriteBlockResponse : public Response
{
public:
	explicit WriteBlockResponse(uint8_t request_sequence_number, uint8_t status);
	std::vector<uint8_t> serialize() const override;
};
