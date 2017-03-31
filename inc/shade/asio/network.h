#pragma once

#include "boost/asio.hpp"
#include "shade/tcp.h"
#include "shade/udp.h"

namespace shade { namespace asio {
	using namespace boost::asio;
	using boost::system::error_code;

	class udp : public shade::udp {
		io_service& service_;
		ip::udp::socket socket_;
	public:
		udp(io_service&, error_code&);
		bool bind(uint16_t) override;
		bool write_datagram(const uint8_t* data, size_t length, uint32_t ip, uint16_t port) override;
		std::unique_ptr<shade::read_handler> read_datagram(std::chrono::milliseconds duration, std::function<void(const uint8_t*, size_t, bool)> && cb) override;
	};

	class tcp : public shade::tcp {
		io_service& service_;
	public:
		tcp(io_service&);
	};
} }
