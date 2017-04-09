#include "menu.h"

namespace menu {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
	auto os_stdin()
	{
		return ::dup(STDIN_FILENO);
	}
#endif

#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
	HANDLE os_stdin() {
#if 0
		HANDLE out = nullptr;
		DuplicateHandle(
			GetCurrentProcess(),
			GetStdHandle(STD_INPUT_HANDLE),
			GetCurrentProcess(),
			&out,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		return out;
#else
		return GetStdHandle(STD_INPUT_HANDLE);
#endif
	}
#endif

	control::control(boost::asio::io_service& service)
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
		: io_{ service, os_stdin() }
#else
		: io_{ service }
#endif
	{
	}

	void control::cancel()
	{
		io_.cancel();
	}

	inline static void hr(size_t length) {
		putc('+', stdout);
		for (size_t i = 0; i < length; ++i)
			putc('-', stdout);
		putc('+', stdout);
		putc('\n', stdout);
	}

	void control::show_menu()
	{
		static constexpr char exit[] = "Exit";

		if (stack_.empty())
			return;

		auto& cur = *stack_.top().menu;
		cur.refresh();

		auto title = cur.title();
		size_t length = title.length + 4;
		size_t count = cur.count();

		for (size_t i = 0; i < count; ++i) {
			if (cur.type(i) == item_type::hidden)
				continue;

			auto text = cur.text(i);
			auto len = 7 + text.length;
			if (len > length)
				length = len;
		}

		auto len = 7 + sizeof(exit) - 1;
		if (len > length)
			length = len;

		printf("\n");
		hr(length);
		printf("| %s:%*c\n", title.text, int(length - title.length - 1), '|');
		hr(length);
		bool has_hr = true;

		auto& mapping = stack_.top().mapping;
		mapping.clear();

		int specials = 0;
		int pos = 0;
		for (size_t i = 0; i < count; ++i) {
			auto type = cur.type(i);
			auto text = cur.text(i);
			switch (type) {
			case item_type::special:
				++specials;
				break;
			case item_type::separator:
				if (!has_hr) {
					hr(length);
					has_hr = true;
				}
				break;
			case item_type::command:
				has_hr = false;
				printf("| %2d: %s%*c\n", ++pos, text.text, int(length - text.length - 4), '|');
				mapping[pos] = i;
				break;
			}
		}

		if (!has_hr)
			hr(length);

		pos = 99 - specials - 1;
		for (size_t i = 0; i < count; ++i) {
			auto type = cur.type(i);
			auto text = cur.text(i);
			switch (type) {
			case item_type::special:
				printf("| %2d: %s%*c\n", ++pos, text.text, int(length - text.length - 4), '|');
				mapping[pos] = i;
				break;
			}
		}

		printf("| 99: %s%*c\n", exit, int(length - sizeof(exit) - 3), '|');
		hr(length);
		printf("> "); fflush(stdout);

		io_.cancel();
		boost::asio::async_read_until(io_, input_buffer_, '\n', [this](boost::system::error_code const& ec, const size_t read) {
			if (!ec)
			{
				auto data = input_buffer_.data();
				auto ptr = boost::asio::buffer_cast<const char*>(data);

				std::string input(ptr, read - 1);
				input_buffer_.consume(read);
				handle_menu(input);
				return;
			}

			if (ec == boost::asio::error::not_found)
			{
				auto data = input_buffer_.data();
				auto ptr = boost::asio::buffer_cast<const char*>(data);

				// Didn't get a newline. Send whatever we have.
				std::string input(ptr, input_buffer_.size());
				input_buffer_.consume(input_buffer_.size());
				handle_menu(input);
			}
		});
	}

	void control::handle_menu(const std::string& input)
	{
		if (!input.empty()) {
			auto const& cur = stack_.top();
			char* end = nullptr;
			auto n = (int)strtol(input.c_str(), &end, 10);
			if (!end || !*end) {
				if (n == 99) {
					stack_.pop();
				} else {
					auto it = cur.mapping.find(n);
					if (it == cur.mapping.end())
						printf("No item #%d...\n", n);
					else
						cur.menu->call(it->second);
				}
			}
		}
		show_menu();
	}
}
