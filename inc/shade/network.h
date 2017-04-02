#pragma once

#include <chrono>
#include <functional>
#include <memory>

namespace shade {
	struct read_handler {
		virtual ~read_handler() = default;
	};
	struct udp {
		virtual ~udp() = default;
		virtual bool bind(uint16_t) = 0;
		virtual bool write_datagram(const uint8_t* data, size_t length, uint32_t ip, uint16_t port) = 0;
		virtual std::unique_ptr<read_handler> read_datagram(std::chrono::milliseconds duration, std::function<void(const uint8_t*, size_t, bool)> && cb) = 0;
	};

	struct tcp {
		virtual ~tcp() = default;
	};

	struct timeout {
		virtual ~timeout() = default;
	};

	using std::chrono::milliseconds;
	struct network {
		virtual ~network() = default;
		virtual std::unique_ptr<udp> udp_socket() = 0;
		virtual std::unique_ptr<tcp> tcp_socket() = 0;
		virtual std::unique_ptr<shade::timeout> timeout(milliseconds duration, std::function<void()> && cb) = 0;
	};
}