#include <shade/discovery.h>
#include <shade/storage.h>
#include <shade/asio/network.h>
#include <iostream>

auto asio_discovery(boost::asio::io_service& service) {
	boost::system::error_code ec;
	auto udp = std::make_unique<shade::asio::udp>(service, ec);
	if (ec) {
		std::cout << "shade-test: " << ec;
		std::exit(2);
	}

	auto tcp = std::make_unique<shade::asio::tcp>(service);
	return shade::discovery{ std::move(udp), std::move(tcp) };
}

int main()
{
	boost::asio::io_service service;
	shade::storage stg;
	stg.load();
	{
		auto const& bridges = stg.bridges();
		if (bridges.empty()) {
			printf("WAITING FOR BRIDGES\n");
		} else {
			for (auto const& bridge : bridges) {
				printf("BRIDGE %s:\n", bridge.first.c_str());
				printf("    base: [%s]\n", bridge.second.base.c_str());
				printf("    username: [%s]\n", bridge.second.username.c_str());
			}
		}
	}
	auto bridges = asio_discovery(service);
	bridges.search([&](auto const& id, auto const& base) {
		if (stg.bridge_located(id, base))
			std::cout << id << ": " << base << "\n";
	});

	service.run();
}
