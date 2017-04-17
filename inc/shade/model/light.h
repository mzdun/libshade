#pragma once

#include <shade/model/light_source.h>
#include <memory>

namespace shade { namespace hue {
	struct light;
} }

namespace shade { namespace model {
	class light : public light_source, public std::enable_shared_from_this<light> {
	public:
		using light_source::light_source;
		bool operator == (const light&) const;
		bool update(const std::string& key, hue::light json);

		static auto make(const std::shared_ptr<model::bridge>& owner, std::string idx, std::string id, std::string name, std::string type, bool on, int bri, color_mode value) {
			return std::make_shared<light>(owner, std::move(idx), std::move(id), std::move(name), std::move(type), on, bri, std::move(value));
		}

		static void prepare(json::struct_translator& tr)
		{
			light_source::prepare(tr);
		}
	};

	inline bool operator!= (const light& lhs, const light& rhs) {
		return !(lhs == rhs);
	}
} }
