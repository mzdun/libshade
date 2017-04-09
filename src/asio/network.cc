#include <shade/asio/network.h>
#include <array>
#include <iostream>

namespace shade { namespace io { namespace asio {
	class udp : public io::udp {
		io_service& service_;
		ip::udp::socket socket_;
	public:
		udp(io_service&, error_code&);
		bool bind(uint16_t) override;
		bool write_datagram(const uint8_t* data, size_t length, uint32_t ip, uint16_t port) override;
		std::unique_ptr<io::read_handler> read_datagram(std::chrono::milliseconds duration, std::function<void(const uint8_t*, size_t, bool)> && cb) override;
	};

	udp::udp(io_service& service, error_code& ec)
		: service_{ service }
		, socket_{ service }
	{
		socket_.open(ip::udp::v4(), ec);
	}

	bool udp::bind(uint16_t port)
	{
		error_code ec;
		socket_.bind(ip::udp::endpoint{ip::udp::v4(), port}, ec);
		return !ec;
	}

	bool udp::write_datagram(const uint8_t* data, size_t length, uint32_t ip, uint16_t port)
	{
		error_code ec;
		auto s = socket_.send_to(
			boost::asio::buffer(data, length),
			ip::udp::endpoint{ ip::address_v4{ ip }, port },
			0, ec);
		return !ec && s == length;
	}

	class datagram_handler : public std::enable_shared_from_this<datagram_handler> {
		static constexpr size_t max_size = 2048;
		bool active_ = false;
		std::array<uint8_t, max_size> data_;
		ip::udp::socket* socket_;
		boost::asio::deadline_timer timer_;
		std::function<void(const uint8_t*, size_t, bool)> cb_;

		auto buffer() { return boost::asio::buffer(data_); }
		void async_read() {
			auto self = shared_from_this();
			socket_->async_receive(buffer(), [self, this](const error_code& ec, std::size_t size) {
				if (ec) {
					cb_(nullptr, 0, ec == boost::asio::error::operation_aborted);
					return;
				}
				cb_(data_.data(), size, true);
				if (active_)
					async_read();
			});
		}

	public:
		datagram_handler(ip::udp::socket* socket, std::function<void(const uint8_t*, size_t, bool)> && cb)
			: socket_{ socket }
			, timer_ { socket->get_io_service() }
			, cb_{ std::move(cb) }
		{
		}

		~datagram_handler()
		{
		}

		void start(std::chrono::milliseconds ms, error_code& ec)
		{
			auto self = shared_from_this();
			timer_.expires_from_now(boost::posix_time::milliseconds{ ms.count() }, ec);
			if (ec) return;
			active_ = true;
			async_read();
			timer_.async_wait([self, this](const error_code& ec) { cancel(true); });
		}

		void cancel(bool /*from_timeout*/)
		{
			active_ = false;

			error_code ec;
			timer_.cancel(ec);
			ec.clear();
			socket_->cancel(ec);
		}
	};

	class read_handler : public io::read_handler {
		std::weak_ptr<datagram_handler> io_;
	public:
		read_handler(const std::shared_ptr<datagram_handler>& io)
			: io_{ io }
		{}
		~read_handler() {
			auto io = io_.lock();
			if (io)
				io->cancel(false);
		}
	};

	std::unique_ptr<io::read_handler> udp::read_datagram(std::chrono::milliseconds duration, std::function<void(const uint8_t*, size_t, bool)> && cb)
	{
		auto io = std::make_shared<datagram_handler>(&socket_, std::move(cb));
		error_code ec;
		io->start(duration, ec);
		if (ec) return {};
		return std::make_unique<read_handler>(io);
	}

	class tcp : public io::tcp {
		io_service& service_;
	public:
		tcp(io_service&);
	};

	tcp::tcp(io_service& service)
		: service_{ service }
	{
	}

	std::unique_ptr<io::udp> network::udp_socket()
	{
		error_code ec;
		auto ptr = std::make_unique<udp>(service_, ec);
		if (ec)
			return {};
		return ptr;
	}

	std::unique_ptr<io::tcp> network::tcp_socket()
	{
		return std::make_unique<tcp>(service_);
	}

	class timer : public std::enable_shared_from_this<timer> {
		deadline_timer timer_;
		std::function<void()> cb_;
	public:
		timer(io_service& service, std::function<void()> && cb)
			: timer_{ service }
			, cb_{ std::move(cb) }
		{}

		bool start(milliseconds duration) {
			error_code ec;
			using posix = boost::posix_time::milliseconds;
			timer_.expires_from_now(posix{duration.count()}, ec);
			if (ec)
				return false;

			auto self = shared_from_this();
			timer_.async_wait([self, this](const error_code& ec) {
				if (ec)
					return;
				cb_();
			});

			return true;
		}

		void cancel() {
			error_code ec;
			timer_.cancel(ec);
		}
	};

	class timeout_handler : public io::timeout {
		std::shared_ptr<timer> timer_;
	public:
		timeout_handler(const std::shared_ptr<timer>& timer)
			: timer_{ timer }
		{}
		~timeout_handler()
		{
			timer_->cancel();
		}
	};

	std::unique_ptr<io::timeout> network::timeout(milliseconds duration, std::function<void()> && cb)
	{
		auto timer = std::make_shared<asio::timer>(service_, std::move(cb));
		auto result = std::make_unique<timeout_handler>(timer);
		if (!timer->start(duration))
			return {};
		return result;
	}
} } }
