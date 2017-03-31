#pragma once
#include <stdint.h>
#include <chrono>

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
}