#pragma once
#include <string>
#include <unordered_set>

namespace json {
	struct struct_translator;
}

namespace shade { namespace listener {
	struct storage;
} }

namespace shade { namespace model {
	class host {
		std::string name_;
		std::string username_;
		std::unordered_set<std::string> selected_;
	public:
		host() = default;
		host(const std::string& name) : name_{ name } {}

		static void prepare(json::struct_translator&);

		const std::string& name() const { return name_; }
		const std::string& username() const { return username_; }
		auto const& selected() const { return selected_; }
		void batch_update(const std::unordered_set<std::string>& upstream) {
			selected_ = upstream;
		}

		bool update(const std::string& user);
		bool is_selected(const std::string& dev) const;
		void switch_selection(const std::string& dev);
	};
} }
