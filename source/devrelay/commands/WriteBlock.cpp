#include "WriteBlock.h"
#include <cstdint>

WriteBlockRequest::WriteBlockRequest(const uint8_t request_sequence_number, const uint8_t device_id, const uint16_t block_size) : Request(request_sequence_number, CMD_WRITE_BLOCK, device_id), block_number_{}, block_data_{}, block_size_(block_size) {}

std::vector<uint8_t> WriteBlockRequest::serialize() const
{
	std::vector<uint8_t> request_data;
	request_data.push_back(this->get_request_sequence_number());
	request_data.push_back(this->get_command_number());
	request_data.push_back(this->get_device_id());
	request_data.push_back(this->get_block_size() & 0xFF);
	request_data.push_back((this->get_block_size() >> 8) & 0xFF);
	request_data.insert(request_data.end(), block_number_.begin(), block_number_.end());
	request_data.insert(request_data.end(), block_data_.begin(), block_data_.end());

	return request_data;
}

std::unique_ptr<Response> WriteBlockRequest::deserialize(const std::vector<uint8_t> &data) const
{
	if (data.size() < 2)
	{
		throw std::runtime_error("Not enough data to deserialize WriteBlockResponse");
	}

	auto response = std::make_unique<WriteBlockResponse>(data[0], data[1]);
	return response;
}

const std::array<uint8_t, 3> &WriteBlockRequest::get_block_number() const { return block_number_; }

const std::vector<uint8_t> &WriteBlockRequest::get_block_data() const { return block_data_; }

const uint16_t WriteBlockRequest::get_block_size() const { return block_size_; }

void WriteBlockRequest::set_block_number_from_ptr(const uint8_t *ptr, const size_t offset) { std::copy_n(ptr + offset, block_number_.size(), block_number_.begin()); }

void WriteBlockRequest::set_block_data_from_ptr(const uint8_t *ptr, const size_t offset) { std::copy_n(ptr + offset, block_data_.size(), block_data_.begin()); }

void WriteBlockRequest::set_block_number_from_bytes(uint8_t l, uint8_t m, uint8_t h)
{
	block_number_[0] = l;
	block_number_[1] = m;
	block_number_[2] = h;
}

WriteBlockResponse::WriteBlockResponse(const uint8_t request_sequence_number, const uint8_t status) : Response(request_sequence_number, status) {}

std::vector<uint8_t> WriteBlockResponse::serialize() const
{
	std::vector<uint8_t> data;
	data.push_back(this->get_request_sequence_number());
	data.push_back(this->get_status());
	return data;
}