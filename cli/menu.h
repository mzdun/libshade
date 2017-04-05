#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <unordered_map>
#include <stack>

namespace menu {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
	using io_type = boost::asio::posix::stream_descriptor;
#endif
#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
	using io_type = boost::asio::windows::stream_handle;
#endif

	enum class item_type {
		command,
		hidden,
		special,
		separator
	};

	struct label {
		const char* text = "";
		size_t length = 0;
		label() = default;
		label(const std::string& s)
			: text(s.c_str())
			, length(s.length())
		{}
	};

	class control;
	struct current {
		virtual ~current() = default;
		virtual void refresh() = 0;
		virtual label title() const = 0;
		virtual size_t count() const = 0;
		virtual item_type type(size_t i) = 0;
		virtual label text(size_t i) = 0;
		virtual void call(size_t i) = 0;
	};

	class control {
	public:
		control(boost::asio::io_service& service);
		void show_menu(std::unique_ptr<current> items)
		{
			stack_.push({ std::move(items) });
			if (stack_.size() == 1)
				show_menu();
		}
	private:
		struct stack {
			std::unique_ptr<current> menu;
			std::unordered_map<int, size_t> mapping;
		};
		void show_menu();
		void handle_menu(const std::string&);

		io_type io_;
		boost::asio::streambuf input_buffer_;
		std::stack<stack> stack_;
	};

	namespace impl {
		template <typename Base = menu::current>
		class simple_title : public Base {
			std::string title_;
		public:
			using title_arg = const std::string&;

			simple_title(title_arg title)
				: title_{ title }
			{
			}

			menu::label title() const override { return title_; }

			void refresh_title() const {}
		};

		template <typename Pred, typename Base = menu::current>
		class dynamic_title : public Base {
			Pred pred_;
		public:
			using title_arg = Pred&&;

			dynamic_title(title_arg pred)
				: pred_{ std::move(pred) }
			{
			}

			menu::label title() const override { return pred_(); }

			void refresh_title() const {}
		};

		struct item {
			virtual ~item() {}
			virtual void refresh() = 0;
			virtual item_type type() = 0;
			virtual label text() = 0;
			virtual void call() = 0;

			static inline std::unique_ptr<item> separator();

			template <typename Pred>
			static inline std::unique_ptr<item> simple(const std::string& text, Pred pred);

			template <typename Pred>
			static inline std::unique_ptr<item> special(const std::string& text, Pred pred);

			template <typename Update, typename Refresh, typename Pred>
			static inline std::unique_ptr<item> simple(Update update, Refresh refresh, Pred pred);

			template <typename Update, typename Refresh, typename Pred>
			static inline std::unique_ptr<item> special(Update update, Refresh refresh, Pred pred);
		};

		class separator : public item {
		public:
			void refresh() override {}
			item_type type() override { return item_type::separator; }
			label text() override { return {}; }
			void call() override { }
		};

		template <item_type Type, typename Pred>
		class callable_item : public item {
			Pred pred_;
		public:
			callable_item(Pred pred)
				: pred_{ std::move(pred) }
			{}
			item_type type() override { return Type; }
			void call() override { pred_(); }
		};

		template <item_type Type, typename Pred>
		class simple_text : public callable_item<Type, Pred> {
			std::string text_;
		public:
			simple_text(const std::string& text, Pred pred)
				: callable_item<Type, Pred>{ std::move(pred) }
				, text_{ text }
			{}
			void refresh() override { }
			label text() override { return text_; }
		};

		template <item_type Type, typename Update, typename Refresh, typename Pred>
		class prop_text : public callable_item<Type, Pred> {
			using prop_type = std::decay_t<decltype(std::declval<Update>()())>;
			Update update_;
			Refresh refresh_;
			prop_type prop_;
			std::string text_;
		public:
			prop_text(Update update, Refresh refresh, Pred pred)
				: callable_item<Type, Pred>{ std::move(pred) }
				, update_{ std::move(update) }
				, refresh_{ std::move(refresh) }
				, prop_ { update_() }
				, text_{ refresh_(prop_) }
			{}
			void refresh() override
			{
				auto prop = update_();
				if (prop == prop_)
					return;
				prop_ = std::move(prop);
				text_ = refresh_(prop_);
			}
			label text() override { return text_; }
		};

		inline std::unique_ptr<item> item::separator()
		{
			return std::make_unique<impl::separator>();
		}

		template <item_type Type, typename Pred>
		static inline std::unique_ptr<item> make_typed_item(const std::string& text, Pred pred)
		{
			return std::make_unique<simple_text<Type, Pred>>(text, std::move(pred));
		}

		template <item_type Type, typename Update, typename Refresh, typename Pred>
		static inline std::unique_ptr<item> make_typed_item(Update update, Refresh refresh, Pred pred)
		{
			return std::make_unique<prop_text<Type, Update, Refresh, Pred>>(std::move(update), std::move(refresh), std::move(pred));
		}

		template <typename Pred>
		inline std::unique_ptr<item> item::simple(const std::string& text, Pred pred)
		{
			return make_typed_item<item_type::command>(text, std::move(pred));
		}

		template <typename Pred>
		inline std::unique_ptr<item> item::special(const std::string& text, Pred pred)
		{
			return make_typed_item<item_type::special>(text, std::move(pred));
		}

		template <typename Update, typename Refresh, typename Pred>
		inline std::unique_ptr<item> item::simple(Update update, Refresh refresh, Pred pred)
		{
			return make_typed_item<item_type::command>(std::move(update), std::move(refresh), std::move(pred));
		}

		template <typename Update, typename Refresh, typename Pred>
		inline std::unique_ptr<item> item::special(Update update, Refresh refresh, Pred pred)
		{
			return make_typed_item<item_type::special>(std::move(update), std::move(refresh), std::move(pred));
		}

		template <typename Base = simple_title<>>
		class current : public Base {
			std::vector<std::unique_ptr<item>> items_;
		public:
			using title_arg = typename Base::title_arg;
			current(title_arg title, std::vector<std::unique_ptr<item>>&& items)
				: Base{ std::forward<title_arg>(title) }
				, items_{ std::move(items) }
			{
			}

			void refresh() override
			{
				this->refresh_title();

				for (auto& item : items_)
					item->refresh();
			}

			size_t count() const override { return items_.size(); }
			menu::item_type type(size_t i) override { return items_[i]->type(); }
			menu::label text(size_t i) override { return items_[i]->text(); }
			void call(size_t i) override { return items_[i]->call(); }
		};
	}
}
