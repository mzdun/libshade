#pragma once

#include <vector>
#include <memory>

namespace json {
	struct struct_translator;
}

namespace shade { namespace hue {
	struct light_state;
} }

namespace shade { namespace model {
	namespace mode {
		enum : int { max_value = 254 };
		struct hue_sat {
			int hue;
			int sat;
		};

		struct ct {
			int val;
		};

		struct xy {
			double x;
			double y;
		};

		enum class mode {
			empty,
			hue_sat,
			ct,
			xy
		};

		class color {
			mode mode_ = mode::empty;
			union {
				hue_sat hue_;
				ct ct_;
				xy xy_;
			};
		public:
			color();
			color(const color&);
			color(color&&);
			color& operator=(const color&);
			color& operator=(color&&);

			color(const hue_sat&);
			color(hue_sat&&);
			color(const ct&);
			color(ct&&);
			color(const xy&);
			color(xy&&);
			bool operator == (const color&) const;
			bool operator != (const color& rhs) const {
				return !(*this == rhs);
			}

			template<class Visitor>
			void visit(Visitor visitor) const {
				switch (mode_) {
				case mode::empty: break;
				case mode::hue_sat: visitor(hue_); break;
				case mode::ct: visitor(ct_); break;
				case mode::xy: visitor(xy_); break;
				}
			}

			template<class Visitor>
			void visit(Visitor visitor) {
				switch (mode_) {
				case mode::empty: break;
				case mode::hue_sat: visitor(hue_); break;
				case mode::ct: visitor(ct_); break;
				case mode::xy: visitor(xy_); break;
				}
			}

			static color from_json(const hue::light_state& state);
		};

		inline int clamp(int v) { return v < 0 ? 0 : v > max_value ? max_value : v; }

		template <typename ... L> struct combine_t;

		template <typename L> struct combine_t<L> : L {
			combine_t(L lambda) : L{ std::move(lambda) } {}
			using L::operator();
		};

		template <typename L1, typename L2, typename ... Ln>
		struct combine_t<L1, L2, Ln...> : L1, combine_t<L2, Ln...> {
			combine_t(L1 l1, L2 l2, Ln ... l) : L1{ std::move(l1) }, combine_t<L2, Ln...>{ std::move(l2), std::move(l)... } {}
			using L1::operator();
			using combine_t<L2, Ln...>::operator();
		};

		template <typename ... L> 
		combine_t<L...> combine(L... l) {
			return { std::move(l)... };
		}
	}
	using color_mode = mode::color;

	class bridge;
	class light_source {
		std::string idx_;
		std::string id_;
		std::string name_;
		std::string type_; // Group type or Light modelid
		bool on_;
		int brightness_;
		color_mode value_;
		std::weak_ptr<model::bridge> owner_;
	public:
		light_source() = default;
		light_source(const std::shared_ptr<model::bridge>& owner, std::string idx, std::string id, std::string name, std::string type, bool on, int bri, color_mode value)
			: idx_{ std::move(idx) }
			, id_{ std::move(id) }
			, name_{ std::move(name) }
			, type_{ std::move(type) }
			, on_{ on }
			, brightness_{ bri }
			, value_{ std::move(value) }
			, owner_{ owner }
		{}
		virtual ~light_source() = default;

		std::shared_ptr<model::bridge> bridge() const { return owner_.lock(); }
		void bridge(const std::shared_ptr<model::bridge>& v) { owner_ = v; }
		const std::string& index() const { return idx_; }
		void index(const std::string& v) { idx_ = v; }
		const std::string& id() const { return id_; }
		void id(const std::string& v) { id_ = v; }
		const std::string& name() const { return name_; }
		void name(const std::string& v) { name_ = v; }
		const std::string& type() const { return type_; }
		void type(const std::string& v) { type_ = v; }

		bool on() const { return on_; }
		void on(bool v) { on_ = v; }
		int bri() const { return brightness_; }
		void bri(int v) { brightness_ = mode::clamp(v); }
		const mode::color& value() const { return value_; }
		void value(const mode::color& v) { value_ = v; }

		virtual bool is_group() const { return false; }

		bool operator == (const light_source&) const;
	protected:
		static void prepare(json::struct_translator&);
	};
} }

namespace shade {
	template <typename T>
	using vector_shared = std::vector<std::shared_ptr<T>>;

	template <typename T>
	inline bool range_equal(const vector_shared<T>& lhs, const vector_shared<T>& rhs) {
		if (lhs.size() != rhs.size())
			return false;

		auto rhs_it = rhs.begin();
		for (auto const& lhs_item : lhs) {
			auto const& rhs_item = *rhs_it++;

			if (!lhs_item)
				return !rhs_item;
			if (!rhs_item)
				return false;

			if (!(*rhs_item == *lhs_item))
				return false;
		}

		return true;
	}
}
