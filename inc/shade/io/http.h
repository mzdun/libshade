#pragma once

#include <tangle/uri.h>
#include <tangle/msg/http_parser.h>
#include <memory>

namespace shade{ namespace io {
	struct http {
		using response = tangle::msg::http_response;
		using headers = response::dict_t;

		struct handler {
			virtual ~handler() = default;
		};

		struct listener {
			virtual ~listener() = default;
			virtual void set_handler(std::unique_ptr<handler>) = 0;
			virtual void on_headers(int status, const tangle::cstring& reason, const headers& headers) = 0;
			virtual void on_data(const char* data, size_t length) = 0;
			virtual tangle::cstring content_type() { return {}; }
		};

		virtual ~http() = default;

		using listener_ptr = std::unique_ptr<listener>;

		auto get(const tangle::uri& address, listener_ptr listener) { return send(method::GET, address, {}, std::move(listener)); }
		auto del(const tangle::uri& address, listener_ptr listener) { return send(method::DEL, address, {}, std::move(listener)); }
		auto put(const tangle::uri& address, const std::string& data, listener_ptr listener) { return send(method::PUT, address, data, std::move(listener)); }
		auto post(const tangle::uri& address, const std::string& data, listener_ptr listener) { return send(method::POST, address, data, std::move(listener)); }

	protected:
		enum class method {
			GET,
			DEL,
			PUT,
			POST
		};

	private:
		virtual bool send(method method, const tangle::uri& address, const std::string& data, listener_ptr listener) = 0;
	};
} }