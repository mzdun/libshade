#pragma once

#include <shade/manager.h>

struct client : shade::manager::client {
	std::unordered_map<std::string, shade::bridge_info> local_bridges_;
	shade::manager* manager_ = nullptr;
	void on_previous(std::unordered_map<std::string, shade::bridge_info> const&) override;
	void on_bridge(std::string const& bridgeid, bool known) override;
	void on_bridge_named(const std::string& bridgeid) override;
	void on_connecting(std::string const& bridgeid) override;
	void on_connected(std::string const& bridgeid) override;
	void on_connect_timeout(std::string const& bridgeid) override;
	void on_refresh(std::string const& bridgeid) override;
};
