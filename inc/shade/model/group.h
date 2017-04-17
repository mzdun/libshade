#pragma once

#include <shade/model/light.h>
#include <memory>

namespace shade { namespace hue {
	struct group;
} }

namespace shade { namespace model {
	class group : public light_source, public std::enable_shared_from_this<group> {
		bool some_;
		std::string klass_;
		vector_shared<light> lights_;
	public:
		group() = default;
		group(std::string id, std::string name, std::string type, std::string klass, bool on, bool some, int bri, color_mode value, vector_shared<light> lights)
			: light_source{ std::move(id), std::move(name), std::move(type), on, bri, std::move(value) }
			, some_{ some }
			, klass_{ std::move(klass) }
			, lights_ { std::move(lights) }
		{}

		const std::string& klass() const { return klass_; }
		void klass(const std::string& v) { klass_ = v; }
		bool some() const { return some_; }
		void some(bool v) { some_ = v; }
		const vector_shared<light>& lights() const { return lights_; }
		void lights(vector_shared<light> v) { lights_ = std::move(v); }
		bool is_group() const override { return true; }
		bool operator == (const group&) const;
		bool update(const std::string& key, hue::group json, const vector_shared<light>& resource);

		static auto make(std::string id, std::string name, std::string type, std::string klass, bool on, bool some, int bri, color_mode value, vector_shared<light> lights) {
			return std::make_shared<group>(std::move(id), std::move(name), std::move(type), std::move(klass), on, some, bri, std::move(value), std::move(lights));
		}

		static void prepare(json::struct_translator& tr);
		static vector_shared<light> referenced(const std::vector<std::string>& refs, const vector_shared<light>& resource);
	};

	inline bool operator!= (const group& lhs, const group& rhs) {
		return !(lhs == rhs);
	}
} }
