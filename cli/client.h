#pragma once
#include "menu.h"
#include <shade/manager.h>
#include <shade/listener.h>

class first_screen;

struct client
	: shade::listener::manager
	, shade::listener::connection
	, shade::listener::bridge
{
	friend class first_screen;

	struct bridge_info {
		std::unordered_set<std::string> selected;
		shade::model::hw_info hw;
	};
	std::unordered_map<std::string, bridge_info> local_;
	std::unordered_map<std::string, std::shared_ptr<shade::heart_monitor>> heartbeats_;
	shade::manager* manager_ = nullptr;
	menu::control menu_;
	bool ignore_heartbeats_ = false;

	client(boost::asio::io_service& service);

	// shade::listener::manager
	void onload(const shade::cache&) override;
	void onbridge(const std::shared_ptr<shade::model::bridge>&) override;
	shade::listener::connection* connection_listener(const std::shared_ptr<shade::model::bridge>&) { return this; }
	shade::listener::bridge* bridge_listener(const std::shared_ptr<shade::model::bridge>&) { return this; }

	// shade::listener::connection
	void onconnecting(const std::shared_ptr<shade::model::bridge>&) override;
	void onconnected(const std::shared_ptr<shade::model::bridge>&) override;
	void onfailed(const std::shared_ptr<shade::model::bridge>&) override;
	void onlost(const std::shared_ptr<shade::model::bridge>&) override;

	// shade::listener::bridge
	void update_start(const std::shared_ptr<shade::model::bridge>&) override;
	void source_added(const std::shared_ptr<shade::model::light_source>&) override;
	void source_removed(const std::shared_ptr<shade::model::light_source>&) override;
	void source_changed(const std::shared_ptr<shade::model::light_source>&) override;
	void update_end(const std::shared_ptr<shade::model::bridge>&) override;

private:
	bool is_heartbeat_on() const;
	void switch_heartbeat();
	void stop_heartbeats();
	void select_items();
};
