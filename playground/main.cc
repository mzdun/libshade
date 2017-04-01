#include <shade/manager.h>
#include <shade/asio/network.h>
#include <iostream>

class client : public shade::manager::client {
	void on_previous(std::unordered_map<std::string, shade::bridge_info> const&) override;
	void on_bridge(std::string const& bridgeid, bool known) override;
	void on_connecting(std::string const& bridgeid) override;
	void on_connected(std::string const& bridgeid) override;
};

int main()
{
	boost::asio::io_service service;
	shade::asio::network net{ service };

	client events;
	shade::manager hue{ &events, &net };
	if (!hue.ready()) {
		std::cout << "shade-test: could not setup the bridge discovery\n";
		return 2;
	}

	hue.search();

	shade::discovery bridges{ &net };


	service.run();
}

void client::on_previous(std::unordered_map<std::string, shade::bridge_info> const& bridges)
{
	if (bridges.empty()) {
		printf("WAITING FOR BRIDGES...\n");
	} else {
		for (auto const& bridge : bridges) {
			printf("BRIDGE %s:\n", bridge.first.c_str());
			printf("    base: [%s]\n", bridge.second.base.c_str());
			printf("    username: [%s]\n", bridge.second.username.c_str());
		}
	}
}

void client::on_bridge(std::string const& bridgeid, bool known)
{
	printf("... >> %s %s\n", known ? "ACTIVATE" : "FOUND", bridgeid.c_str());
}

void client::on_connecting(std::string const&)
{
	printf("PLEASE PRESS THE BUTTON...\n");
}

void client::on_connected(std::string const&)
{
	printf("THANK YOU\n");
}
