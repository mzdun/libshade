#include "shade/manager.h"
#include "shade/io/connection.h"
#include "json.hpp"
#include "manager_internal.h"
#include <algorithm>

using namespace std::literals;

namespace shade {
	manager::manager(const std::string& name, listener* listener, io::network* net, io::http* browser)
		: listener_{ listener }
		, net_{ net }
		, browser_{ browser }
		, storage_{ name }
	{
		storage_.load();
	}

	void manager::search()
	{
		listener_->on_previous(storage_.bridges());

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

	const model::bridge& manager::bridge(const std::string& bridgeid) const
	{
		auto it = storage_.find(bridgeid);
		if (it != storage_.end())
			return it->second;
		static const model::bridge dummy;
		return dummy;
	}

	bool manager::reconnect(json::value doc, const io::connection& conn)
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

	void manager::get_config(const io::connection& conn)
	{
		conn.get("/config", io::make_client([=](int status, json::value doc) {
			api::config cfg;
			if (unpack_json(cfg, doc)) {
				storage_.bridge_named(conn.id(), cfg.name, cfg.mac, cfg.modelid);
				listener_->on_bridge(conn.id());
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

	void manager::create_user(const io::connection& conn)
	{
		listener_->on_connecting(conn.id());
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
	std::string userdefinition(const std::string& listener) {
		auto name = machine();
		if (name.empty())
			name = "unknown";

		return json::map{}.add("devicetype", listener + "#" + name).to_string();
	}

	void manager::get_user(const io::connection& conn, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then) {
		if (status / 100 < 4) {
			auto value = get_username(doc);
			if (value.is<json::STRING>()) {
				auto username = value.as<json::STRING>();
				storage_.bridge_connected(conn.id(), username);
				listener_->on_connected(conn.id());
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
		listener_->on_connect_timeout(conn.id());
	}

	void manager::create_user(const io::connection& conn, std::chrono::nanoseconds sofar)
	{
		if (sofar >= 30s) {
			listener_->on_connect_timeout(conn.id());
			return;
		}

		auto then = std::chrono::steady_clock::now();
		auto json = userdefinition(storage_.current_client());
		conn.post("", json, io::make_json_client([=](int status, json::value doc) {
			get_user(conn, status, doc, sofar, then);
		}));
	}

	void manager::get_lights(const io::connection& conn)
	{
		conn.get("/lights", io::make_client([=](int status, json::value doc) {
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

	void manager::get_groups(const io::connection& conn, std::unordered_map<std::string, api::light> lights)
	{
		conn.get("/groups", io::make_client([=, lights = std::move(lights)](int status, json::value doc) {
			std::unordered_map<std::string, api::group> groups;
			if (unpack_json(groups, doc)) {
				add_config(conn.id(), std::move(lights), std::move(groups));
				listener_->on_refresh(conn.id());
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
			for (auto& lightref : group.second.lights) {
				auto it = lights.find(lightref);
				if (it == lights.end()) {
					lightref.clear();
				} else
					lightref = it->second.uniqueid;
			}
		}

		vector_shared<model::light> stg_lights;
		stg_lights.reserve(lights.size());
		std::transform(begin(lights), end(lights), std::back_inserter(stg_lights), [](auto & in) {
			model::color_mode mode;
			if (in.second.state.colormode == "hs")
				mode = model::mode::hue_sat{ in.second.state.hue, in.second.state.sat };
			else if(in.second.state.colormode == "xy")
				mode = model::mode::xy{ in.second.state.xy[0], in.second.state.xy[1] };
			else if (in.second.state.colormode == "ct")
				mode = model::mode::ct{ in.second.state.ct };

			return model::light::make(
				std::move(in.second.uniqueid),
				std::move(in.second.name),
				std::move(in.second.modelid),
				in.second.state.on,
				in.second.state.bri,
				mode);
		});

		vector_shared<model::group> stg_groups;
		stg_groups.reserve(groups.size());
		std::transform(begin(groups), end(groups), std::back_inserter(stg_groups), [&](auto & in) {
			model::color_mode mode;
			if (in.second.action.colormode == "hs")
				mode = model::mode::hue_sat{ in.second.action.hue, in.second.action.sat };
			else if (in.second.action.colormode == "xy")
				mode = model::mode::xy{ in.second.action.xy[0], in.second.action.xy[1] };
			else if (in.second.action.colormode == "ct")
				mode = model::mode::ct{ in.second.action.ct };

			vector_shared<model::light> refs;
			refs.reserve(in.second.lights.size());
			for (auto const& light : in.second.lights) {
				for (auto& ref : stg_lights) {
					if (ref->id() == light)
						refs.push_back(ref);
				}
			}

			return model::group::make(
				"group/" + in.first,
				std::move(in.second.name),
				std::move(in.second.type),
				std::move(in.second.klass),
				in.second.state.all_on,
				in.second.state.any_on,
				in.second.action.bri,
				mode,
				std::move(refs));
		});

		storage_.bridge_config(bridgeid, std::move(stg_lights), std::move(stg_groups));
	}

}
