#pragma once

#include <tangle/uri.h>
#include <tangle/msg/http_parser.h>
#include <memory>

namespace shade{
	struct http {
		using response = tangle::msg::http_response;
		using headers = response::dict_t;

		struct handler {
			virtual ~handler() = default;
		};

		struct client {
			virtual ~client() = default;
			virtual void set_handler(std::unique_ptr<handler>) = 0;
			virtual void on_headers(int status, const tangle::cstring& reason, const headers& headers) = 0;
			virtual void on_data(const char* data, size_t length) = 0;
			virtual tangle::cstring content_type() { return {}; }
		};

		virtual ~http() = default;

		using client_ptr = std::unique_ptr<client>;

		auto get(const tangle::uri& address, client_ptr client) { return send(method::GET, address, {}, std::move(client)); }
		auto del(const tangle::uri& address, client_ptr client) { return send(method::DEL, address, {}, std::move(client)); }
		auto put(const tangle::uri& address, const std::string& data, client_ptr client) { return send(method::PUT, address, data, std::move(client)); }
		auto post(const tangle::uri& address, const std::string& data, client_ptr client) { return send(method::POST, address, data, std::move(client)); }

	protected:
		enum class method {
			GET,
			DEL,
			PUT,
			POST
		};

	private:
		virtual bool send(method method, const tangle::uri& address, const std::string& data, client_ptr client) = 0;
	};
}