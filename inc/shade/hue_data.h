#pragma once
#include <string>
#include <array>
#include <vector>

namespace shade { namespace hue {
	struct config {
		std::string name;
		std::string mac;
		std::string modelid;
	};

	struct group_state {
		bool all_on;
		bool any_on;
	};

	struct light_state {
		bool on;
		int bri;
		int hue;
		int sat;
		int ct;
		std::array<double, 2> xy;
		std::string colormode;
	};

	struct group {
		std::string name;
		std::string type;
		std::string klass;
		std::vector<std::string> lights;
		group_state state;
		light_state action;
	};

	struct light {
		std::string name;
		std::string modelid;
		std::string uniqueid;
		light_state state;
	};

	struct error_type {
		int type;
		std::string address;
		std::string description;
	};

	enum class errors {
		unauthorised_user = 1,
		invalid_json = 2,
		resource_404 = 3,
		method_unsupported = 4,
		missing_param = 5,
		parameter_404 = 6,
		invalid_value = 7,
		param_read_only = 8,
		too_many_items = 11,
		portal_required = 12,
		button_not_pressed = 101,
		internal_error = 901
	};
} }
