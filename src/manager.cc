#include "shade/manager.h"

namespace shade {
	manager::manager(client* client, network* net)
		: client_{ client }
		, net_{ net }
	{
		storage_.load();
	}

	void manager::search()
	{
		client_->on_previous(storage_.bridges());

		discovery_.search([this](auto const& id, auto const& base) {
			const auto known = !storage_.bridge_located(id, base);
			client_->on_bridge(id, known);
		});
	}
}
