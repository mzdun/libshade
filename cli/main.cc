#include "client.h"
#include <shade/asio/network.h>
#include <shade/asio/http.h>
#include <shade/version.h>
#include <iostream>

int main() try {
	boost::asio::io_service service;
	shade::io::asio::network net{ service };
	shade::io::asio::http browser{ service };

	client events{ service };
	shade::manager hue{ "shade-cli", &events, &net, &browser };
	if (!hue.ready()) {
		std::cout << "shade-cli: could not setup the bridge discovery\n";
		return 2;
	}

	printf("shade-cli version %s (%s)\n\n", SHADE_VERSION_FULL, SHADE_VERSION_VCS);
	events.manager_ = &hue;
	hue.search();

	service.run();
} catch (std::exception const & ex) {
	printf("shade-cli: %s\n", ex.what());
}
