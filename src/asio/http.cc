#include <shade/asio/http.h>

namespace shade { namespace asio {
	class http_handler : public http::handler {
		io_service& service_;
		ip::tcp::resolver resolver_;
		ip::tcp::socket socket_;
		streambuf request_;
		streambuf response_;
		http::client_ptr client_;

		void error(const error_code& ec) {
			client_->on_headers(ec.value(), {}, {});
			client_->on_data(nullptr, 0);
		}
		void error(int status = 500) {
			client_->on_headers(status, {}, {});
			client_->on_data(nullptr, 0);
		}

		void connect(ip::tcp::resolver::iterator endpoints);
		void write_request();
		void read_headers();
		void read_body();
	public:

		http_handler(io_service& service, http::client_ptr client)
			: service_{ service }
			, resolver_{ service }
			, socket_{ service }
			, client_{ std::move(client) }
		{}

		std::streambuf* buffer() { return &request_; }
		http::client* client() { return client_.get(); }

		void send(const std::string& host, const std::string& service);
	};

	static inline bool error(http::client* client, int status = 1000) {
		client->on_headers(status, {}, {});
		client->on_data(nullptr, 0);
		return false;
	}

	bool http::send(method method, const tangle::uri& address, const std::string& data, client_ptr client)
	{
		auto handler = std::make_unique<http_handler>(service_, std::move(client));
		auto content_type = handler->client()->content_type();
		switch (method) {
		case method::PUT:
		case method::POST:
			if (content_type.empty() || data.empty())
				return error(handler->client());
			break;
		default:
			if (!data.empty())
				return error(handler->client());
		}

		auto auth = tangle::uri::auth_builder::parse(address.authority());

		std::ostream os(handler->buffer());
		switch (method) {
		case method::GET:  os << "GET "; break;
		case method::DEL:  os << "DELETE "; break;
		case method::PUT:  os << "PUT "; break;
		case method::POST: os << "POST "; break;
		}
		os << address.path() << address.query() << " HTTP/1.0\r\n";
		os << "Host: " << auth.host;
		if (!auth.port.empty())
			os << ":" << auth.port;
		os << "\r\n";
		os << "Accept: */*\r\n";
		if (!data.empty()) {
			os << "Content-Type: " << content_type << "\r\n";
			os << "Content-Length: " << data.length() << "\r\n";
		}
		os << "Connection: close\r\n\r\n";
		os << data;

		auto service = auth.port.empty() ? address.scheme().str() : auth.port;
		auto ptr = handler.get();
		ptr->client()->set_handler(std::move(handler));
		ptr->send(auth.host, service);
		return true;
	}

	void http_handler::send(const std::string& host, const std::string& service)
	{
		ip::tcp::resolver::query query{ host, service };
		resolver_.async_resolve(query, [this](const error_code& ec, ip::tcp::resolver::iterator endpoints) {
			if (ec)
				return error(ec);
			connect(endpoints);
		});
	}

	void http_handler::connect(ip::tcp::resolver::iterator endpoints)
	{
		async_connect(socket_, endpoints, [this](const error_code& ec, ip::tcp::resolver::iterator iterator) {
			if (ec)
				return error(ec);
			write_request();
		});
	}

	void http_handler::write_request()
	{
		async_write(socket_, request_, [this](const error_code& ec, size_t written) {
			if (ec)
				return error(ec);
			read_headers();
		});
	}

	void http_handler::read_headers()
	{
		async_read_until(socket_, response_, "\r\n\r\n", [this](const error_code& ec, size_t read) {
			if (ec)
				return error(ec);

			auto data = response_.data();
			auto ptr = buffer_cast<const char*>(data);

			using namespace tangle::msg;
			http_response parser{};
			auto result = parser.append(ptr, read);
			if (std::get<parsing>(result) != parsing::separator) {
				socket_.close();
				return error();
			}

			response_.consume(std::get<size_t>(result) + 2);

			auto headers = parser.dict();
			client_->on_headers(parser.status(), parser.reason(), headers);

			if (response_.size() > 0) {
				auto data = response_.data();
				auto size = buffer_size(data);
				auto ptr = buffer_cast<const char*>(data);
				if (size) {
					client_->on_data(ptr, size);
					response_.consume(size);
				}
			}

			read_body();
		});
	}

	void http_handler::read_body()
	{
		async_read(socket_, response_, transfer_at_least(1), [this](const error_code& ec, size_t read) {
			if (ec)
				return client_->on_data(nullptr, 0);

			auto data = response_.data();
			auto size = buffer_size(data);
			auto ptr = buffer_cast<const char*>(data);
			if (size) {
				client_->on_data(ptr, size);
				response_.consume(size);
			}

			read_body();
		});
	}

} }
