#include "client.h"
#include <shade/asio/network.h>
#include <shade/asio/http.h>
#include <iostream>

int main()
{
	boost::asio::io_service service;
	shade::asio::network net{ service };
	shade::asio::http browser{ service };

	client events;
	shade::manager hue{ &events, &net, &browser };
	if (!hue.ready()) {
		std::cout << "shade-test: could not setup the bridge discovery\n";
		return 2;
	}

	events.manager_ = &hue;
	hue.search();

	service.run();
}
