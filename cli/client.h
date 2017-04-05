#pragma once
#include "menu.h"
#include <shade/manager.h>

class first_screen;
struct client : shade::manager::client {
	friend class first_screen;

	std::unordered_map<std::string, shade::bridge_info> local_bridges_;
	shade::manager* manager_ = nullptr;
	menu::control menu_;

	client(boost::asio::io_service& service);
	void on_previous(std::unordered_map<std::string, shade::bridge_info> const&) override;
	void on_bridge(std::string const& bridgeid, bool known) override;
	void on_bridge_named(const std::string& bridgeid) override;
	void on_connecting(std::string const& bridgeid) override;
	void on_connected(std::string const& bridgeid) override;
	void on_connect_timeout(std::string const& bridgeid) override;
	void on_refresh(std::string const& bridgeid) override;

private:
	void select_items();
	void switch_device(const std::string& bridgeid, const std::string& id);
};
