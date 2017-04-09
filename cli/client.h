#pragma once
#include "menu.h"
#include <shade/manager.h>

class first_screen;
struct client : shade::manager::listener {
	friend class first_screen;

	std::unordered_map<std::string, shade::model::bridge> local_bridges_;
	shade::manager* manager_ = nullptr;
	menu::control menu_;

	client(boost::asio::io_service& service);
	void on_previous(std::unordered_map<std::string, shade::model::bridge> const&) override;
	void on_bridge(const std::string& bridgeid) override;
	void on_connecting(std::string const& bridgeid) override;
	void on_connected(std::string const& bridgeid) override;
	void on_connect_timeout(std::string const& bridgeid) override;
	void on_refresh(std::string const& bridgeid) override;

private:
	void select_items();
};
