#pragma once
#include <boost/asio.hpp>
#include <shade/http.h>

namespace shade { namespace asio {
	using namespace boost::asio;
	using boost::system::error_code;

	class http : public shade::http {
		io_service& service_;
		bool send(method method, const tangle::uri& address, const std::string& data, client_ptr client) override;
	public:
		http(io_service& service)
			: service_{ service }
		{}
	};
} }
