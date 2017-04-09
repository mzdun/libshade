#pragma once

#include "boost/asio.hpp"
#include "shade/io/network.h"

namespace shade { namespace io { namespace asio {
	using namespace boost::asio;
	using boost::system::error_code;

	class network : public io::network {
		io_service& service_;
	public:
		network(io_service& service) : service_{ service } {}
		std::unique_ptr<io::udp> udp_socket() override;
		std::unique_ptr<io::tcp> tcp_socket() override;
		std::unique_ptr<io::timeout> timeout(milliseconds duration, std::function<void()> && cb) override;
	};
} } }
