#include <shade/discovery.h>
#include <shade/storage.h>
#include <shade/asio/network.h>
#include <iostream>

int main()
{
	boost::asio::io_service service;
	shade::asio::network net{ service };

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

	shade::discovery bridges{ &net };
	if (!bridges.ready()) {
		std::cout << "shade-test: could not setup the bridge discovery\n";
		return 2;
	}

	bridges.search([&](auto const& id, auto const& base) {
		if (stg.bridge_located(id, base))
			std::cout << id << ": " << base << "\n";
	});

	service.run();
}
