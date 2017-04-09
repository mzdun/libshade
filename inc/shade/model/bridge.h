#pragma once
#include <shade/model/host.h>
#include <shade/model/base.h>
#include <vector>

namespace json {
	struct struct_translator;
}

namespace shade { namespace model {
	struct bridge {
		bool seen = false;
		hw_info hw;
		std::vector<light_info> lights;
		std::vector<group_info> groups;

		static void prepare(json::struct_translator&);

		void set_host(const std::string& name, storage* stg);
		void set_host_from(const host& name);
		void rebind_current();
		host* get_current() const { return current_; }
	private:
		host* current_ = nullptr;
		std::vector<host> hosts_;
	};
} }
