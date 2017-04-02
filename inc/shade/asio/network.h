#pragma once

#include "boost/asio.hpp"
#include "shade/network.h"

namespace shade { namespace asio {
	using namespace boost::asio;
	using boost::system::error_code;

	class network : public shade::network {
		io_service& service_;
	public:
		network(io_service& service) : service_{ service } {}
		std::unique_ptr<shade::udp> udp_socket() override;
		std::unique_ptr<shade::tcp> tcp_socket() override;
		std::unique_ptr<shade::timeout> timeout(milliseconds duration, std::function<void()> && cb) override;
	};
} }
