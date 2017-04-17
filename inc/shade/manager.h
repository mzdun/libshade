#pragma once

#include <shade/discovery.h>
#include <shade/cache.h>
#include <shade/io/http.h>
#include <json.hpp>

namespace shade {
	namespace hue {
		struct light;
		struct group;
	}

	namespace io {
		class connection;
	}

	namespace listener {
		struct manager;
	}

	struct heart_monitor {
		virtual ~heart_monitor() = default;
	};

	class manager;
	class change_def {
		friend class manager;
		json::map opts_;
		void erase(const std::string& key)
		{
			auto it = opts_.find(key);
			if (it != opts_.end())
				opts_.erase(it);
		}
	public:
		change_def& on(bool val)
		{
			opts_.add("on", val);
			return *this;
		}

		change_def& bri(int val)
		{
			opts_.add("bri", model::mode::clamp(val));
			return *this;
		}

		change_def& color(const model::color_mode& color)
		{
			erase("hue");
			erase("sat");
			erase("xy");
			erase("ct");
			color.visit(model::mode::combine(
				[&](const model::mode::hue_sat& hs) { opts_.add("hue", hs.hue).add("sat", hs.sat); },
				[&](const model::mode::xy& xy) { json::vector v; v.add(xy.x).add(xy.y); opts_.add("xy", v); },
				[&](const model::mode::ct& ct) { opts_.add("ct", ct.val); }
			));
			return *this;
		}

		change_def& erase_on()
		{
			erase("on");
			return *this;
		}

		change_def& erase_bri()
		{
			erase("bri");
			return *this;
		}

		change_def& erase_color()
		{
			return color({});
		}
	};

	class manager {
	public:
		manager(const std::string& name, listener::manager* listener, io::network* net, io::http* browser);

		const auto& current_host() const { return view_.current_host(); }
		bool ready() const { return discovery_.ready(); }
		const cache& view() const { return view_; }
		void store_cache();
		void search();
		void connect(const std::shared_ptr<model::bridge>&);
		std::shared_ptr<heart_monitor> defib(const std::shared_ptr<model::bridge>&);
		void update(const std::shared_ptr<shade::model::light_source>& source, const change_def& change);
	private:
		listener::manager* listener_;
		io::network* net_;
		cache view_;
		discovery discovery_{ net_ };
		std::unordered_map<std::string, std::unique_ptr<io::timeout>> timeouts_;

		void get_config(const io::connection& conn);

		void connect(const std::shared_ptr<model::bridge>&, std::chrono::nanoseconds sofar);
		void getuser(const std::shared_ptr<model::bridge>& bridge, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then);

		void do_update(const std::shared_ptr<shade::model::light_source>& source, const change_def& change);
	};
}
