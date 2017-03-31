#include "shade/discovery.h"
#include "tangle/msg/http_parser.h"
#include "tangle/uri.h"

#define DISCOVERY_TIMEOUT 3
#define STR2(x) #x
#define STR(x) STR2(x)

namespace shade {
	namespace {
		class listener {
		};
	}
	discovery::discovery(std::unique_ptr<udp> udp, std::unique_ptr<tcp> tcp)
		: udp_socket_(std::move(udp))
		, tcp_socket_(std::move(tcp))
	{
		uint16_t port = 2000;
		unsigned int tries = 0;
		constexpr unsigned int maxtries = 10;

		while (!udp_socket_->bind(port++)) {
			if (++tries == maxtries) return;
		}

		conntected_ = true;
	}

	discovery::discovery(discovery&&) = default;
	discovery& discovery::operator=(discovery&&) = default;
	discovery::~discovery() = default;

	class discovery_handler {
		discovery* parent;
		discovery::onbridge callback;
	public:
		discovery_handler(discovery* parent, discovery::onbridge callback)
			: parent{ parent }
			, callback{ std::move(callback) }
		{
		}

		void on_packet(const uint8_t* data, size_t length)
		{
			tangle::msg::http_response parser{};

			tangle::msg::parsing result;
			std::tie(std::ignore, result) = parser.append((const char*)data, length);
			if (result == tangle::msg::parsing::reading)
				std::tie(std::ignore, result) = parser.append("\r\n", 2);

			if (result != tangle::msg::parsing::separator)
				return;

			auto dict = parser.dict();

			auto it = dict.find("hue-bridgeid");
			if (it == dict.end() || it->second.empty())
				return;
			auto bridgeid = it->second.front();

			it = dict.find("location");
			if (it == dict.end() || it->second.empty())
				return;
			auto location = tangle::uri::normal(it->second.front(), tangle::uri::with_pass).string();

			// 3. note the id as "seen"
			if (parent->seen(bridgeid, location))
				return;

			// 4. get the contents of the xml
			// 5. extract the base address
			// TODO: actually take the description.xml contents...
			tangle::uri uri{ location };
			uri.fragment({});
			uri.query({});
			uri.path({});
			// 6. for each bridge report the (id, base address) to callback
			callback(bridgeid, uri.string());
		}
	};

	bool discovery::search(onbridge callback)
	{
		current_search_.reset();

		constexpr std::chrono::seconds ssdp_timeout{ DISCOVERY_TIMEOUT };
		constexpr uint32_t ssdp_ip = 0xEFFFFFFA; // 239.255.255.250
		constexpr uint16_t ssdp_port = 1900;
		constexpr uint8_t packet[] =
			"M-SEARCH * HTTP/1.1\r\n"
			"HOST: 239.255.255.250:1900\r\n"
			"MAN: \"ssdp:discover\"\r\n"
			"MX: " STR(DISCOVERY_TIMEOUT) "\r\n"
			"ST: libhue:idl\r\n";

		// 1. send out a packet, in triplicate
		if (!udp_socket_->write_datagram(packet, sizeof(packet) - 1, ssdp_ip, ssdp_port))
			return false;
		if (!udp_socket_->write_datagram(packet, sizeof(packet) - 1, ssdp_ip, ssdp_port))
			return false;
		if (!udp_socket_->write_datagram(packet, sizeof(packet) - 1, ssdp_ip, ssdp_port))
			return false;

		// 2. gather the (id, location) pair
		auto handler = std::make_shared<discovery_handler>(this, std::move(callback));
		current_search_ = udp_socket_->read_datagram(ssdp_timeout, [handler](const uint8_t* data, size_t length, bool success) {
			if (!success) {
				printf("FAILED\n");
				return;
			}
			if (length == 0) {
				printf("DONE\n");
				return;
			}
			handler->on_packet(data, length);
		});
		return true;
	}

	bool discovery::seen(const std::string& bridgeid, const std::string& location)
	{
		auto known = known_bridges_.find(bridgeid);
		if (known != known_bridges_.end())
			return true;

		known_bridges_[bridgeid].location = location;
		return false;
	}
}
