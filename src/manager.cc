#include <shade/manager.h>
#include <shade/listener.h>
#include <shade/storage.h>
#include <shade/io/connection.h>
#include <json.hpp>
#include "manager_internal.h"
#include <algorithm>

using namespace std::literals;

namespace shade {
	class storage_listener : public listener::storage {
		bool dirty_ = false;
		cache* parent_;
	public:
		storage_listener(cache* parent) : parent_{ parent } {}
		~storage_listener() {
			if (dirty_)
				shade::storage::store(*parent_);
		}

		void mark_dirty() { dirty_ = true; }
	};

	manager::manager(const std::string& name, listener::manager* listener, io::network* net, io::http* browser)
		: listener_{ listener }
		, net_{ net }
		, view_{ name, browser }
	{
		storage::load(view_);
	}

	void manager::search()
	{
		listener_->onload(view_);

		discovery_.search([this](std::string const& id, std::string const& base) {
			storage_listener listener{ &view_ };
			view_.bridge_located(id, base, &listener);
			auto bridge = view_.get(id);
			if (!bridge)
				return;

			get_config(bridge->unlogged(view_.browser()));
		}, [this] {
			// retry all unactivated cached bridges
			for (auto const& bridge : view_) {
				if (bridge.second->seen()) continue;
				get_config(bridge.second->unlogged(view_.browser()));
			}
		});
	}

	void manager::store_cache()
	{
		storage::store(view_);
	}

	void manager::get_config(const io::connection& conn)
	{
		conn.get("/config", io::make_client([=](int status, json::value doc) {
			hue::config cfg;
			if (unpack_json(cfg, doc)) {
				auto bridge = view_.get(conn.id());
				if (!bridge)
					return;
				storage_listener listener{ &view_ };
				view_.bridge_named(
					conn.id(),
					std::move(cfg.name),
					std::move(cfg.mac),
					std::move(cfg.modelid),
					&listener
				);
				listener_->onbridge(bridge);
			}
		}));
	}

	void manager::connect(const std::shared_ptr<model::bridge>& bridge)
	{
		connect(bridge, {});
	}

	std::string machine();
	std::string userdefinition(const std::string& listener) {
		auto name = machine();
		if (name.empty())
			name = "unknown";

		return json::map{}.add("devicetype", listener + "#" + name).to_string();
	}

	void manager::connect(const std::shared_ptr<model::bridge>& bridge, std::chrono::nanoseconds sofar)
	{
		auto listener = listener_->connection_listener(bridge);
		if (!sofar.count() && listener) {
			listener->onconnecting(bridge);
		}

		if (sofar >= 30s) {
			if (listener)
				listener->onfailed(bridge);
			return;
		}

		auto then = std::chrono::steady_clock::now();
		auto json = userdefinition(view_.current_host());
		bridge->unlogged(view_.browser()).post("", json, io::make_json_client([=](int status, json::value doc) {
			getuser(bridge, status, doc, sofar, then);
		}));
	}

	static inline json::value username(json::value doc)
	{
		json::value out;
		find_first(doc, [&](json::value child) {
			out = map(map(child, "success"), "username");
			return !out.is<json::NULLPTR>();
		});
		return out;
	}

	void manager::getuser(const std::shared_ptr<model::bridge>& bridge, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then)
	{
		auto listener = listener_->connection_listener(bridge);

		if (status / 100 < 4) {
			auto value = username(doc);
			if (value.is<json::STRING>()) {
				auto username = value.as<json::STRING>();
				bridge->host().update(username);
				store_cache();
				if (listener)
					listener->onconnected(bridge);
				return;
			}
		}

		hue::errors error;
		if (get_error(error, doc)) {
			if (error == hue::errors::button_not_pressed) {
				auto& timeout = timeouts_[bridge->id()];
				timeout = net_->timeout(300ms, [=]() {
					timeouts_.erase(bridge->id());
					auto now = std::chrono::steady_clock::now();
					connect(bridge, sofar + (now - then));
				});

				if (timeout)
					return;
			}
		}

		if (listener)
			listener->onfailed(bridge);
	}

	bool manager::reconnect(json::value doc, const std::shared_ptr<model::bridge>& bridge)
	{
		hue::errors error;
		if (get_error(error, doc)) {
			if (error == hue::errors::unauthorised_user) {
				auto listener = listener_->connection_listener(bridge);
				if (listener)
					listener->onlost(bridge);
				return true;
			}
		}
		return false;
	}

	void manager::get_lights(const std::shared_ptr<model::bridge>& bridge)
	{
		bridge->logged(view_.browser()).get("/lights", io::make_client([=](int status, json::value doc) {
			std::unordered_map<std::string, hue::light> lights;
			if (unpack_json(lights, doc)) {
				get_groups(bridge, std::move(lights));
				return;
			}

			if (reconnect(doc, bridge))
				return;

			printf("GETTING LIGHTS: %d\n%s\n", status, doc.to_string(json::value::options::indented()).c_str());
		}));
	}

	void manager::get_groups(const std::shared_ptr<model::bridge>& bridge, std::unordered_map<std::string, hue::light> lights)
	{
		bridge->logged(view_.browser()).get("/groups", io::make_client([=, lights = std::move(lights)](int status, json::value doc) {
			std::unordered_map<std::string, hue::group> groups;
			if (unpack_json(groups, doc)) {
				storage_listener listener{ &view_ };
				view_.bridge_lights(bridge, std::move(lights), std::move(groups), &listener);
				return;
			}

			if (reconnect(doc, bridge))
				return;

			printf("GETTING GROUPS: %d\n%s\n", status, doc.to_string(json::value::options::indented()).c_str());
		}));
	}

}
