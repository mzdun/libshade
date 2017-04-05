#include "client.h"
#include <shade/asio/network.h>
#include <shade/asio/http.h>
#include <iostream>

int main() try {
	boost::asio::io_service service;
	shade::asio::network net{ service };
	shade::asio::http browser{ service };

	client events{ service };
	shade::manager hue{ &events, &net, &browser };
	if (!hue.ready()) {
		std::cout << "shade-cli: could not setup the bridge discovery\n";
		return 2;
	}

	events.manager_ = &hue;
	hue.search();

	service.run();
} catch (std::exception const & ex) {
	printf("shade-cli: %s\n", ex.what());
}
