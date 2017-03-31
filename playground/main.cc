#include <shade/discovery.h>
#include <iostream>

auto asio_discovery() {
	return std::make_unique<shade::discovery>(nullptr, nullptr);
}

int main()
{
	auto bridges = asio_discovery();
	bridges->search([](auto const& id, auto const& base) {
		std::cout << id << ": " << base << "\n";
	});
}
