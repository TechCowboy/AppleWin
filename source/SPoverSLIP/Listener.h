#pragma once

#include <memory>
#include <mutex>
#include <regex>
#include <cstdint>
#include <string>
#include <thread>

#include "Connection.h"

class Listener
{
public:
  // Listener(std::string ip_address, const uint16_t port);
  Listener();
  ~Listener();

  void Initialize(std::string ip_address, const uint16_t port);

  void start();
  void stop();

  std::thread create_listener_thread();
  bool get_is_listening() const;

  std::pair<uint8_t, std::shared_ptr<Connection>> find_connection_with_device(const uint8_t device_id) const;
  std::vector<std::pair<uint8_t, Connection *>> get_all_connections() const;

  void insert_connection(uint8_t start_id, uint8_t end_id, const std::shared_ptr<Connection> &conn);

  static uint8_t get_total_device_count();
  void set_start_on_init(bool should_start) { should_start_ = should_start; }
  bool get_start_on_init() { return should_start_; }
  std::pair<int, int> first_two_disk_devices() const;

  // default values for listener
  bool default_start_listener = true;
  std::string default_listener_address = "0.0.0.0";
  //std::array<char, 16> default_listener_address_array = { "0.0.0.0" };
  uint16_t default_port = 1985;

  std::string get_ip_address() const { return ip_address_; }
  std::string check_and_set_ip_address(const std::string& ip_address) {
	if (is_valid_ip_address(ip_address)) {
		ip_address_ = ip_address;
	} else {
		ip_address_ = default_listener_address;
	}
	return ip_address_;
  }

  uint16_t get_port() const { return port_; }
  void set_port(uint16_t port) { port_ = port; }


private:
  std::string ip_address_;
  uint16_t port_;
  std::mutex mtx_;
  std::thread listening_thread_;
  static uint8_t next_device_id_;
  std::map<std::pair<uint8_t, uint8_t>, std::shared_ptr<Connection>> connection_map_;
  mutable std::pair<int, int> cached_disk_devices;
  mutable bool cache_valid = false;

  bool is_listening_;
  bool should_start_;
  void create_connection(unsigned int socket);
  void listener_function();
  bool is_valid_ip_address(const std::string &ipAddress) { return std::regex_match(ipAddress, ipPattern); }

  static const std::regex ipPattern;

};