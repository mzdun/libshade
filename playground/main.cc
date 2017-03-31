#include <shade/discovery.h>
#include <shade/asio/network.h>
#include <iostream>

auto asio_discovery(boost::asio::io_service& service, boost::system::error_code& ec) {
	auto udp = std::make_unique<shade::asio::udp>(service, ec);
	if (ec)
		return std::unique_ptr<shade::discovery>{};

	auto tcp = std::make_unique<shade::asio::tcp>(service);
	return std::make_unique<shade::discovery>(std::move(udp), std::move(tcp));
}

int main()
{
	boost::asio::io_service service;
	boost::system::error_code ec;
	auto bridges = asio_discovery(service, ec);
	if (ec) {
		std::cout << "shade-test: " << ec;
		return 2;
	}

	bridges->search([](auto const& id, auto const& base) {
		std::cout << id << ": " << base << "\n";
	});

	service.run();
}
