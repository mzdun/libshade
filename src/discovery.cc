#include "shade/discovery.h"

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
	}

	discovery::discovery(discovery&&) = default;
	discovery& discovery::operator=(discovery&&) = default;
	discovery::~discovery() = default;

	bool discovery::search(onbridge callback)
	{
		constexpr uint32_t sddp_ip = 0xEFFFFFFA; // 239.255.255.250
		constexpr uint16_t sddp_port = 1900;
		constexpr uint8_t packet[] =
			"M-SEARCH * HTTP/1.1\r\n"
			"HOST: 239.255.255.250:1900\r\n"
			"MAN: \"ssdp:discover\"\r\n"
			"MX: " STR(DISCOVERY_TIMEOUT) "\r\n"
			"ST: libhue:idl\r\n";

		// 1. send out a packet, in triplicate
		if (!udp_socket_->write_datagram(packet, sizeof(packet) - 1, sddp_ip, sddp_port))
			return false;
		if (!udp_socket_->write_datagram(packet, sizeof(packet) - 1, sddp_ip, sddp_port))
			return false;
		if (!udp_socket_->write_datagram(packet, sizeof(packet) - 1, sddp_ip, sddp_port))
			return false;

		// 2. gather the (id, location) pair
		// 3. note the id as seen
		// 4. get the contents of the xml
		// 5. extract the base address
		// 6. for each bridge report the (id, base address) to callback
		return false;
	}
}
