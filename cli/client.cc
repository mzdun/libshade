#include "client.h"
#include <iostream>

void print(const shade::light_info& light, bool is_new)
{
	printf("        [%s]: '%s' (%s) %s [%d]", light.id.c_str(),
		light.name.c_str(), light.type.c_str(),
		light.state.on ? "ON" : "OFF",
		light.state.bri);

	if (is_new) printf(" NEW LIGHT");
	printf("\n");
}

void print(const shade::group_info& group, bool is_new)
{
	printf("        [%s]: '%s' (%s) %s [%d]", group.id.c_str(),
		group.name.c_str(),
		group.type == "Room" ? group.klass.c_str() : group.type.c_str(),
		group.state.any ? (group.state.on ? "ON" : "SOME") : "OFF",
		group.state.bri);

	if (is_new) printf(" NEW GROUP");
	printf("\n");
}

void client::on_previous(std::unordered_map<std::string, shade::bridge_info> const& bridges)
{
	if (bridges.empty()) {
		printf("WAITING FOR BRIDGES...\n");
	} else {
		local_bridges_ = bridges;
		for (auto const& bridge : bridges) {
			printf("BRIDGE %s:\n", bridge.first.c_str());
			printf("    base:     %s\n", bridge.second.base.c_str());
			printf("    username: %s\n", bridge.second.username.c_str());
			printf("    name:     %s\n", bridge.second.name.c_str());
			printf("    mac:      %s\n", bridge.second.mac.c_str());
			printf("    modelid:  %s\n", bridge.second.modelid.c_str());
			if (!bridge.second.lights.empty()) {
				printf("    lights:\n");
				for (auto const& light : bridge.second.lights) {
					print(light, false);
				}
			}
			if (!bridge.second.groups.empty()) {
				printf("    groups:\n");
				for (auto const& group : bridge.second.groups) {
					print(group, false);
					for (auto& ref : group.lights) {
						printf("            -> %s\n", ref.c_str());
					}
				}
			}
		}
	}
}

void client::on_bridge(std::string const& bridgeid, bool known)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("... >> %s %s\n", known ? "ACTIVATING" : "FOUND", bridgeid.c_str());
	if (bridge.base != local.base) {
		printf("    base: [%s]\n", bridge.base.c_str());
		local.base = bridge.base;
	}
}

void client::on_bridge_named(std::string const& bridgeid)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("... >> GOT BASIC CONFIG\n");
	if (bridge.name != local.name) {
		printf("    name: [%s]\n", bridge.name.c_str());
		local.name = bridge.name;
	}
	if (bridge.mac != local.mac) {
		printf("    mac: [%s]\n", bridge.mac.c_str());
		local.mac = bridge.mac;
	}
	if (bridge.modelid != local.modelid) {
		printf("    modelid: [%s]\n", bridge.modelid.c_str());
		local.modelid = bridge.modelid;
	}
}

void client::on_connecting(std::string const&)
{
	printf("PLEASE PRESS THE BUTTON...\n");
}

void client::on_connected(std::string const& bridgeid)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("THANK YOU.\n");
	printf("    username: [%s]\n", bridge.username.c_str());
}

void client::on_connect_timeout(std::string const& bridgeid)
{
	printf("BUTTON NOT PRESSED IN 30s.\n");
}

template <typename T>
static auto find_if(const T& container, const std::string& id)
{
	using std::begin;
	using std::end;

	return std::find_if(begin(container), end(container), [&](auto const& item) { return item.id == id; });
}

template <typename T>
void print_list(T& local, const T& update, const char* title)
{
	using std::begin;
	using std::end;

	bool removed = false;
	bool header = false;
	for (auto& loc : local) {
		auto it = find_if(update, loc.id);
		if (it == end(update)) {
			if (!header) {
				header = true;
				printf("    %s:\n", title);
			}
			printf("        [%s] REMOVED\n", loc.id.c_str());
			removed = true;
			continue;
		}
		if (*it != loc) {
			loc = *it;
			if (!header) {
				header = true;
				printf("    %s:\n", title);
			}
			print(loc, false);
		}
	}

	if (removed) {
		auto it = std::remove_if(begin(local), end(local), [&](const auto& item) { return find_if(update, item.id) == end(update); });
		local.erase(it, end(local));
		local.shrink_to_fit();
	}

	for (auto const& upd : update) {
		auto it = find_if(local, upd.id);
		if (it == end(local)) {
			if (!header) {
				header = true;
				printf("    %s:\n", title);
			}
			print(upd, true);
			local.push_back(upd);
			continue;
		}
	}
}

void client::on_refresh(std::string const& bridgeid)
{
	auto const& bridge = manager_->bridge(bridgeid);
	auto& local = local_bridges_[bridgeid];
	printf("... >> GOT CURRENT CONFIG\n");
	print_list(local.lights, bridge.lights, "lights");
	print_list(local.groups, bridge.groups, "groups");
}
