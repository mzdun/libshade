#pragma once
#include <stdint.h>

namespace shade {
	struct udp {
		virtual ~udp() = default;
		virtual bool bind(uint16_t) = 0;
		virtual bool write_datagram(const uint8_t* data, size_t length, uint32_t ip, uint16_t port) = 0;
	};
}