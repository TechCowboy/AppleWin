#pragma once

#include "Connection.h"
#include <memory>
#include <string>

class TCPConnection : public Connection, public std::enable_shared_from_this<TCPConnection>
{
public:
  TCPConnection(int socket) : socket_(socket) {}

  virtual void send_data(const std::vector<uint8_t> &data) override;
  virtual void create_read_channel() override;
  virtual void close_connection() override;

  int get_socket() const { return socket_; }
  void set_socket(int socket) { this->socket_ = socket; }

private:
  int socket_;
};
