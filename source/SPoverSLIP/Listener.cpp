
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "Listener.h"

#include "InitRequest.h"
#include "InitResponse.h"
#include "SLIP.h"
#include "TCPConnection.h"
#include "StatusRequest.h"
#include "StatusResponse.h"

#include "Log.h"
#include "Requestor.h"

// clang-format off
#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  #define CLOSE_SOCKET closesocket
  #define SOCKET_ERROR_CODE WSAGetLastError()
#else
  #include <arpa/inet.h>
  #include <errno.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <unistd.h>
#include "StatusRequest.h"
#include "StatusRequest.h"
  #define CLOSE_SOCKET close
  #define SOCKET_ERROR_CODE errno
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
#endif
// clang-format off

uint8_t Listener::next_device_id_ = 1;
const std::regex Listener::ipPattern("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

Listener& GetSPoverSLIPListener(void)
{
  static Listener listener;
  return listener;
}

Listener::Listener() : is_listening_(false) {}

void Listener::Initialize(std::string ip_address, const uint16_t port)
{
  ip_address_ = std::move(ip_address);
  port_ = port;
}

bool Listener::get_is_listening() const { return is_listening_; }

void Listener::insert_connection(uint8_t start_id, uint8_t end_id, const std::shared_ptr<Connection> &conn)
{
  connection_map_[{start_id, end_id}] = conn;
}

uint8_t Listener::get_total_device_count() { return next_device_id_ - 1; }

Listener::~Listener() { stop(); }

std::thread Listener::create_listener_thread() { return std::thread(&Listener::listener_function, this); }

void Listener::listener_function()
{
  LogFileOutput("Listener::listener_function - RUNNING\n");
  int server_fd, new_socket;
  struct sockaddr_in address;
#ifdef WIN32
  int address_length = sizeof(address);
#else
  socklen_t address_length = sizeof(address);
#endif

#ifdef WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    LogFileOutput("WSAStartup failed: %d\n", WSAGetLastError());
  }
#endif

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
  {
    LogFileOutput("Listener::listener_function - Socket creation failed\n");
    return;
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(port_);
  inet_pton(AF_INET, ip_address_.c_str(), &(address.sin_addr));

#ifdef WIN32
  char opt = 1;
#else
  int opt = 1;
#endif
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SOCKET_ERROR)
  {
    LogFileOutput("Listener::listener_function - setsockopt failed\n");
    return;
  }
#ifdef WIN32
  if (bind(server_fd, reinterpret_cast<SOCKADDR *>(&address), sizeof(address)) == SOCKET_ERROR)
#else
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
#endif
  {
    LogFileOutput("Listener::listener_function - bind failed\n");
    return;
  }

  if (listen(server_fd, 3) < 0)
  {
    LogFileOutput("Listener::listener_function - listen failed\n");
    return;
  }

  while (is_listening_)
  {
    fd_set sock_set;
    FD_ZERO(&sock_set);
    FD_SET(server_fd, &sock_set);

    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    const int activity = select(server_fd + 1, &sock_set, nullptr, nullptr, &timeout);

    if (activity == SOCKET_ERROR)
    {
      LogFileOutput("Listener::listener_function - select failed\n");
      is_listening_ = false;
      break;
    }

    if (activity == 0)
    {
      // timeout occurred, no client connection. Still need to check is_listening_
      continue;
    }

#ifdef WIN32
    if ((new_socket = accept(server_fd, reinterpret_cast<SOCKADDR *>(&address), &address_length)) == INVALID_SOCKET)
#else
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &address_length)) == INVALID_SOCKET)
#endif
    {
      LogFileOutput("Listener::listener_function - accept failed\n");
      is_listening_ = false;
      break;
    }

    create_connection(new_socket);
  }

  LogFileOutput("Listener::listener_function - listener closing down\n");

  CLOSE_SOCKET(server_fd);
}

// Creates a Connection object, which is how SP device(s) will register itself with our listener.
void Listener::create_connection(unsigned int socket)
{
  // Create a connection, give it some time to settle, else exit without creating listener to connection
  const std::shared_ptr<Connection> conn = std::make_shared<TCPConnection>(socket);
  conn->create_read_channel();

  const auto start = std::chrono::steady_clock::now();
  // Give the connection a generous 10 seconds to work.
  constexpr auto timeout = std::chrono::seconds(10);

  while (!conn->is_connected())
  {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - start) > timeout)
    {
      LogFileOutput("Listener::create_connection() - Failed to establish "
                    "connection, timed out.\n");
      return;
    }
  }

  // We need to send an INIT to device 01 for this connection, then 02, ...
  // until we get an error back This will determine the number of devices
  // attached.

  bool still_scanning = true;
  uint8_t unit_id = 1;

  // send init requests to find all the devices on this connection, or we have too many devices.
  while (still_scanning && (unit_id + next_device_id_) < 255)
  {
    LogFileOutput("SmartPortOverSlip listener sending request for unit_id: %d\n", unit_id);
    InitRequest request(Requestor::next_request_number(), unit_id);
    const auto response = Requestor::send_request(request, conn.get());
    const auto init_response = dynamic_cast<InitResponse *>(response.get());
    if (init_response == nullptr)
    {
      LogFileOutput("SmartPortOverSlip listener ERROR, no response data found\n");
      break;
    }
    still_scanning = init_response->get_status() == 0;
    if (still_scanning)
      unit_id++;
  }

  const auto start_id = next_device_id_;
  const auto end_id = static_cast<uint8_t>(start_id + unit_id - 1);
  next_device_id_ = end_id + 1;
  // track the connection and device ranges it reported. Further connections can add to the devices we can target.
  LogFileOutput("SmartPortOverSlip listener creating connection for start: %d, end: %d\n", start_id, end_id);
  insert_connection(start_id, end_id, conn);
}

void Listener::start()
{
  is_listening_ = true;
  listening_thread_ = std::thread(&Listener::listener_function, this);
}

void Listener::stop()
{
  LogFileOutput("Listener::stop()\n");
  if (is_listening_)
  {
    // Stop listener first, otherwise the PC might reboot too fast and be picked up
    is_listening_ = false;
    LogFileOutput("Listener::stop() ... joining listener until it stops\n");
    listening_thread_.join();

    LogFileOutput("Listener::stop() - closing %ld connections\n", connection_map_.size());
    for (auto &pair : connection_map_)
    {
      const auto &connection = pair.second;
      connection->set_is_connected(false);
      connection->close_connection();
      connection->join();
    }
  }
  next_device_id_ = 1;
  connection_map_.clear();

#ifdef WIN32
  WSACleanup();
#endif

  LogFileOutput("Listener::stop() ... finished\n");
}

// Returns the ADJUSTED lower bound of the device id, and connection.
// i.e. the ID that the target thinks the device index is.
// We store (for example) device_ids in applewin with values 1-5 for connection 1, 6-8 for connection 2, but each device things they are 1-5, and 1-3 (not 6-8).
// However the apple side sees 1-8, and so we have to convert 6, 7, 8 into the target's 1, 2, 3
std::pair<uint8_t, std::shared_ptr<Connection>> Listener::find_connection_with_device(const uint8_t device_id) const
{
  std::pair<uint8_t, std::shared_ptr<Connection>> result;
  for (const auto &kv : connection_map_)
  {
    if (device_id >= kv.first.first && device_id <= kv.first.second)
    {
      result = std::make_pair(device_id - kv.first.first + 1, kv.second);
      break;
    }
  }
  return result;
}

std::vector<std::pair<uint8_t, Connection *>> Listener::get_all_connections() const
{
  std::vector<std::pair<uint8_t, Connection *>> connections;
  for (const auto &kv : connection_map_)
  {
    for (uint8_t id = kv.first.first; id <= kv.first.second; ++id)
    {
      connections.emplace_back(id, kv.second.get());
    }
  }
  return connections;
}

std::pair<int, int> Listener::first_two_disk_devices() const {
    if (cache_valid) {
        return cached_disk_devices;
    }

    cached_disk_devices = {-1, -1}; // Initialize with invalid device ids

    for (uint8_t unit_number = 1; unit_number < next_device_id_; ++unit_number) {
        const auto id_and_connection = GetSPoverSLIPListener().find_connection_with_device(unit_number);
        if (id_and_connection.second == nullptr) continue;

        // ids from the map need adjusting down to their real ids on the device


        // DIB request to get information block
        const StatusRequest request(Requestor::next_request_number(), unit_number, 3);


        std::unique_ptr<Response> response = Requestor::send_request(request, id_and_connection.second.get());

        // Cast the Response to a StatusResponse
        StatusResponse* statusResponse = dynamic_cast<StatusResponse*>(response.get());

        if (statusResponse) {
            const std::vector<uint8_t>& data = statusResponse->get_data();

            // Check if data size is at least 22 and the device type is a disk, and that the disk status is ONLINE (status[0] bit 4)
            if (data.size() >= 22 && (data[21] == 0x01 || data[21] == 0x02 || data[21] == 0x0A) && ((data[0] & 0x10) == 0x10)) {
                // If first disk device id is not set, set it
                if (cached_disk_devices.first == -1) {
                    cached_disk_devices.first = unit_number;
                }
                // Else if second disk device id is not set, set it and break the loop
                else if (cached_disk_devices.second == -1) {
                    cached_disk_devices.second = unit_number;
                    break;
                }
            }
        }
    }

    cache_valid = true;
    return cached_disk_devices;
}