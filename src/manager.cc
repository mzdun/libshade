#include "shade/manager.h"
#include "shade/connection.h"
#include "json.hpp"
#include "manager_internal.h"
#include <algorithm>

using namespace std::literals;

namespace shade {
	manager::manager(const std::string& name, client* client, network* net, http* browser)
		: client_{ client }
		, net_{ net }
		, browser_{ browser }
		, storage_{ name }
	{
		storage_.load();
	}

	void manager::search()
	{
		client_->on_previous(storage_.bridges());

		discovery_.search([this](std::string const& id, std::string const& base) {
			const auto known = !storage_.bridge_located(id, base);

			auto it = storage_.find(id);
			if (it == storage_.end())
				return;

			get_config({ browser_, base, id });
		}, [this] {
			// retry all unactivated cached bridges
			for (auto const& bridge : storage_) {
				if (bridge.second.seen) continue;
				get_config({ browser_, bridge.second.hw.base, bridge.first });
			}
		});
	}

	const bridge_info& manager::bridge(const std::string& bridgeid) const
	{
		auto it = storage_.find(bridgeid);
		if (it != storage_.end())
			return it->second;
		static const bridge_info dummy;
		return dummy;
	}

	bool manager::reconnect(json::value doc, const connection& conn)
	{
		api::errors error;
		if (get_error(error, doc)) {
			if (error == api::errors::unauthorised_user) {
				create_user(conn.unlogged());
				return true;
			}
		}
		return false;
	}

	void manager::get_config(const connection& conn)
	{
		conn.get("/config", make_client([=](int status, json::value doc) {
			api::config cfg;
			if (unpack_json(cfg, doc)) {
				client_->on_bridge(conn.id(), storage_.find(conn.id()) != storage_.end());
				storage_.bridge_named(conn.id(), cfg.name, cfg.mac, cfg.modelid);
				client_->on_bridge_named(conn.id());
				auto it = storage_.find(conn.id());
				if (it == storage_.end())
					return;

				auto const& username = it->second.get_current()->username();
				if (username.empty())
					create_user(conn);
				else
					get_lights(conn.logged(username));
			}
		}));
	}

	void manager::create_user(const connection& conn)
	{
		client_->on_connecting(conn.id());
		create_user(conn, {});
	}

	static inline json::value get_username(json::value doc)
	{
		json::value out;
		find_first(doc, [&](json::value child) {
			out = map(map(child, "success"), "username");
			return !out.is<json::NULLPTR>();
		});
		return out;
	}

	std::string machine();
	std::string userdefinition(const std::string& client) {
		auto name = machine();
		if (name.empty())
			name = "unknown";

		return json::map{}.add("devicetype", client + "#" + name).to_string();
	}

	void manager::get_user(const connection& conn, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then) {
		if (status / 100 < 4) {
			auto value = get_username(doc);
			if (value.is<json::STRING>()) {
				auto username = value.as<json::STRING>();
				storage_.bridge_connected(conn.id(), username);
				client_->on_connected(conn.id());
				get_lights(conn.logged(username));
				return;
			}
		}

		api::errors error;
		if (get_error(error, doc)) {
			if (error == api::errors::button_not_pressed) {
				auto& timeout = timeouts_[conn.id()];
				timeout = net_->timeout(300ms, [=]() {
					timeouts_.erase(conn.id());
					auto now = std::chrono::steady_clock::now();
					create_user(conn, sofar + (now - then));
				});

				if (timeout)
					return;
			}
		}
		client_->on_connect_timeout(conn.id());
	}

	void manager::create_user(const connection& conn, std::chrono::nanoseconds sofar)
	{
		if (sofar >= 30s) {
			client_->on_connect_timeout(conn.id());
			return;
		}

		auto then = std::chrono::steady_clock::now();
		auto json = userdefinition(storage_.current_client());
		conn.post("", json, make_json_client([=](int status, json::value doc) {
			get_user(conn, status, doc, sofar, then);
		}));
	}

	void manager::get_lights(const connection& conn)
	{
		conn.get("/lights", make_client([=](int status, json::value doc) {
			std::unordered_map<std::string, api::light> lights;
			if (unpack_json(lights, doc)) {
				get_groups(conn, std::move(lights));
				return;
			}

			if (reconnect(doc, conn))
				return;

			printf("GETTING LIGHTS: %d\n%s\n", status, doc.to_string(json::value::options::indented()).c_str());
		}));
	}

	void manager::get_groups(const connection& conn, std::unordered_map<std::string, api::light> lights)
	{
		conn.get("/groups", make_client([=, lights = std::move(lights)](int status, json::value doc) {
			std::unordered_map<std::string, api::group> groups;
			if (unpack_json(groups, doc)) {
				add_config(conn.id(), std::move(lights), std::move(groups));
				client_->on_refresh(conn.id());
				return;
			}

			if (reconnect(doc, conn))
				return;

			printf("GETTING GROUPS: %d\n%s\n", status, doc.to_string(json::value::options::indented()).c_str());
		}));
	}

	void manager::add_config(const std::string& bridgeid, std::unordered_map<std::string, api::light> lights, std::unordered_map<std::string, api::group> groups) {
		using std::begin;
		using std::end;

		for (auto& group : groups) {
			bool erase_refs = false;
			for (auto& lightref : group.second.lights) {
				auto it = lights.find(lightref);
				if (it == lights.end()) {
					lightref.clear();
					erase_refs = true;
				} else
					lightref = it->second.uniqueid;
			}

			if (erase_refs) {
				// ...
			}
		}

		std::vector<light_info> stg_lights;
		stg_lights.reserve(lights.size());
		std::transform(begin(lights), end(lights), std::back_inserter(stg_lights), [](auto & in) {
			light_info out;
			out.id = std::move(in.second.uniqueid);
			out.name = std::move(in.second.name);
			out.type = std::move(in.second.modelid);
			out.state.on = in.second.state.on;
			out.state.bri = in.second.state.bri;
			out.state.hue = in.second.state.hue;
			out.state.sat = in.second.state.sat;
			out.state.ct = in.second.state.ct;
			out.state.xy[0] = in.second.state.xy[0];
			out.state.xy[1] = in.second.state.xy[1];
			out.state.mode = std::move(in.second.state.colormode);
			return out;
		});

		std::vector<group_info> stg_groups;
		stg_groups.reserve(groups.size());
		std::transform(begin(groups), end(groups), std::back_inserter(stg_groups), [](auto & in) {
			group_info out;
			out.id = "group/" + in.first;
			out.name = std::move(in.second.name);
			out.type = std::move(in.second.type);
			out.klass = std::move(in.second.klass);
			out.state.any = in.second.state.any_on;
			out.state.on = in.second.state.all_on;
			out.state.bri = in.second.action.bri;
			out.state.hue = in.second.action.hue;
			out.state.sat = in.second.action.sat;
			out.state.ct = in.second.action.ct;
			out.state.xy[0] = in.second.action.xy[0];
			out.state.xy[1] = in.second.action.xy[1];
			out.state.mode = std::move(in.second.action.colormode);
			out.lights = std::move(in.second.lights);
			return out;
		});

		storage_.bridge_config(bridgeid, std::move(stg_lights), std::move(stg_groups));
	}

}
