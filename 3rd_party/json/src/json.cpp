/*
 * Copyright (C) 2014 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "json.hpp"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <net/utf8.hpp>
#if defined _WIN32 || defined WIN32
#include <Windows.h>
#endif

namespace json
{
	void json_string(std::ostream& o, const std::string& s) {
		auto w = utf::widen(s);
		o << '"';
		for (auto c : w) {
			switch (c){
			case '"' : o << "\\\""; break;
			case '\\': o << "\\\\"; break;
			//case '/' : o << "\\/"; break;
			case '\b': o << "\\b"; break;
			case '\f': o << "\\f"; break;
			case '\n': o << "\\n"; break;
			case '\r': o << "\\r"; break;
			case '\t': o << "\\t"; break;
			default:
				if (c >= 0x80)
					o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << c << std::dec;
				else
					o << (uint8_t)c;
			}
		}
		o << '"';
	};

	std::string value::to_string(const options& options) const
	{
		std::ostringstream o;
		to_string(o, options, 0);
		return o.str();
	}

	std::string value::to_html(const options& options) const
	{
		std::ostringstream o;
		to_html(o, options, 0);
		return o.str();
	}

	std::string make_indent(const value::options& opts, int offset) {
		if (!opts.val.doIndent)
			return{};

		auto spaces = opts.val.indent * offset;
		std::string indent;
		//indent.reserve(spaces + 1);
		indent.push_back('\n');

		for (int i = 0; i < spaces; ++i)
			indent.append(opts.val.indentStr);

		return indent;
	}

	void vector::backend::to_string(std::ostream& o, const options& opts, int offset) const
	{
		auto indent = make_indent(opts, ++offset);
		o << '[';

		bool first = true;
		for (auto&& value : values) {
			if (first) first = false;
			else o << opts.val.listsep;

			if (opts.val.doIndent)
				o << indent;
			value.to_string(o, opts, offset);
		}
		o << make_indent(opts, offset - 1) << ']';
	}

	void vector::backend::to_html(std::ostream& o, const options& opts, int offset) const
	{
		auto indent = make_indent(opts, ++offset);
		o << '[';

		bool first = true;
		for (auto&& value : values) {
			if (first) first = false;
			else o << opts.val.listsep;

			if (opts.val.doIndent)
				o << indent;
			value.to_html(o, opts, offset);
		}
		o << make_indent(opts, offset - 1) << ']';
	}

	map& map::add(const std::string& k, const value& v)
	{
		auto it = find(k);
		if (it != end()) {
			it->second = v;
			return *this;
		}

		values().insert(std::make_pair(k, v));

		return *this;
	}

	void map::backend::to_string(std::ostream& o, const options& opts, int offset) const
	{
		auto indent = make_indent(opts, ++offset);
		o << '{';
		bool first = true;
		for (auto&& pair : values) {
			if (first) first = false;
			else o << opts.val.listsep;

			if (opts.val.doIndent)
				o << indent;
			json_string(o, pair.first);
			o << opts.val.namesep;
			pair.second.to_string(o, opts, offset);
		}
		o << make_indent(opts, offset - 1) << '}';
	}

	void map::backend::to_html(std::ostream& o, const options& opts, int offset) const
	{
		auto indent = make_indent(opts, ++offset);
		o << '{';
		bool first = true;
		for (auto&& pair : values) {
			if (first) first = false;
			else o << opts.val.listsep;

			if (opts.val.doIndent)
				o << indent;
			o << "<span class='cpp-string'>";
			json_string(o, pair.first);
			o << "</span>";
			o << opts.val.namesep;
			pair.second.to_html(o, opts, offset);
		}
		o << make_indent(opts, offset - 1) << '}';
	}

	namespace parser
	{
		enum token {
			JSON_ERROR,
			JSON_NULL,
			JSON_TRUE,
			JSON_FALSE,
			JSON_STRING,
			JSON_NUMBER,
			JSON_ARRAY_START,
			JSON_ARRAY_END,
			JSON_OBJECT_START,
			JSON_OBJECT_END,
			JSON_COLON,
			JSON_COMMA
		};

		struct token_t;
		struct pos_t {
			int line;
			int column;
			pos_t() : line(1), column(1) {}

			void enter() { column = 1; ++line; }
			void next() { ++column; }
			token_t err(const char* msg) const;
			token_t tok(token t) const;
			token_t str(const std::string&) const;
			token_t num(const std::string&) const;
		};

		struct token_t {
			token first = JSON_NULL;
			std::string second;
			pos_t pos;

			token_t() {}
			token_t(token t, const pos_t& pos) : first(t), pos(pos) {}
			token_t(token t, const std::string& s, const pos_t& pos) : first(t), second(s), pos(pos) {}
			token_t(const token_t&) = default;
			token_t& operator=(const token_t&) = default;
			token_t(token_t&&) = default;
			token_t& operator=(token_t&&) = default;
		};

		token_t pos_t::err(const char* msg) const {
			std::ostringstream o;
			o << "(" << line << ',' << column << "): error: " << msg << "\n";
#if defined _WIN32 || defined WIN32
			OutputDebugStringA(o.str().c_str());
#endif
			std::cerr << o.str() << std::flush;
			return tok(JSON_ERROR);
		}

		token_t pos_t::tok(token t) const { return{ t, *this }; }
		token_t pos_t::str(const std::string& s) const { return{ JSON_STRING, s, *this }; }
		token_t pos_t::num(const std::string& s) const { return{ JSON_NUMBER, s, *this }; }

		struct input {
			const char* m_cur;
			const char* m_end;
			pos_t pos;

			token_t tok(token t) const { return pos.tok(t); }
			token_t str(const std::string& s) const { return pos.str(s); }
			token_t num(const std::string& s) const { return pos.num(s); }

			input& operator ++() {
				if (*m_cur == '\n') pos.enter();
				else pos.next();

				++m_cur;
				return *this;
			}

			explicit operator bool() const {
				return m_cur != m_end;
			}

			char cur() const { return *m_cur; }

			token_t err(const char* msg) const { return pos.err(msg); }
		};

		using tokens_t = std::vector<token_t>;

//#define err(x) in.__err(__FILE__, __LINE__, x)

		token_t json_string(input& in) {
			auto pos = in.pos;

			++in;
			std::string value;
			uint16_t unicode;
			while (in) {
				if (in.cur() == '\\') {
					++in;
					if (!in)
						return in.err("String not finished");
					switch (in.cur()) {
					case '"': value.push_back('\"'); break;
					case '\\': value.push_back('\\'); break;
					case '/': value.push_back('/'); break;
					case 'b': value.push_back('\b'); break;
					case 'f': value.push_back('\f'); break;
					case 'n': value.push_back('\n'); break;
					case 'r': value.push_back('\r'); break;
					case 't': value.push_back('\t'); break;
					case 'u':
						unicode = 0;
						for (int i = 0; i < 4; ++i) {
							++in;
							if (!in)
								return in.err("Expecting HEX");

							unicode <<= 4;
							switch (in.cur()) {
							case '0': case '1': case '2': case '3': case '4':
							case '5': case '6': case '7': case '8': case '9':
								unicode += in.cur() - '0';
								break;
							case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
								unicode += in.cur() - 'a' + 10;
								break;
							case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
								unicode += in.cur() - 'A' + 10;
								break;
							default:
								return in.err("Expecting HEX");
							};
						}

						{
							std::u16string codepoint{ unicode, 0 };
							value += utf::narrowed(codepoint);
						}
						break;
					default:
						value.push_back('\\');
						value.push_back((uint8_t)in.cur());
					}
				}
				else if (in.cur() == '"') {
					++in;
					break;
				}
				else value.push_back((uint8_t)in.cur());
				++in;
			}
			return pos.str(value);
		}

		token_t json_number(input& in) {
			auto save = in.m_cur;
			auto pos = in.pos;

			if (in.cur() == '-')
				++in;
			while (in && std::isdigit((uint8_t)in.cur()))
				++in;

			return pos.num({ save, in.m_cur });
		}

		token_t json_token(token t, const char* name, input& in) {

			while (in && *name && *name == in.cur())
				++name, ++in;

			if (*name)
				return in.err("Invalid token");

			return in.tok(t);
		}

		tokens_t tokenize(input& in, bool& error) {
			error = false;
			tokens_t ret;
			while (in) {
				while (in && std::isspace((uint8_t)in.cur()))
					++in;

				if (!in)
					break;

				switch (in.cur()) {
				case '[': ret.push_back(in.tok(JSON_ARRAY_START)); ++in; break;
				case ']': ret.push_back(in.tok(JSON_ARRAY_END)); ++in; break;
				case '{': ret.push_back(in.tok(JSON_OBJECT_START)); ++in; break;
				case '}': ret.push_back(in.tok(JSON_OBJECT_END)); ++in; break;
				case ':': ret.push_back(in.tok(JSON_COLON)); ++in; break;
				case ',': ret.push_back(in.tok(JSON_COMMA)); ++in; break;
				case 'n': ret.push_back(json_token(JSON_NULL, "null", in)); break;
				case 't': ret.push_back(json_token(JSON_TRUE, "true", in)); break;
				case 'f': ret.push_back(json_token(JSON_FALSE, "false", in)); break;
				case '"': ret.push_back(json_string(in)); break;
				case '-':
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					ret.push_back(json_number(in));
					break;
				default:
					ret.push_back(in.err("Unknown character"));
					error = true;
					return ret;
				}
			}
			return ret;
		}

		value array(tokens_t::const_iterator& cur, tokens_t::const_iterator end, bool& error);
		value dict(tokens_t::const_iterator& cur, tokens_t::const_iterator end, bool& error);

		value parse(tokens_t::const_iterator& cur, tokens_t::const_iterator end, bool& error) {
			if (cur == end)
				return{};

			auto tok = *cur++;
			switch (tok.first) {
			case JSON_NULL: return{};
			case JSON_TRUE: return true;
			case JSON_FALSE: return false;
			case JSON_STRING: return tok.second;
			case JSON_NUMBER: return (std::int64_t)std::stoll(tok.second);
			case JSON_ARRAY_START: return array(cur, end, error);
			case JSON_OBJECT_START: return dict(cur, end, error);
			default:
				break;
			}

			error = true;
			tok.pos.err("Unexpected token");
			return{};
		}

		value array(tokens_t::const_iterator& cur, tokens_t::const_iterator end, bool& error) {
			vector out;
			if (cur != end && cur->first == JSON_ARRAY_END) {
				++cur;
				return out;
			}

			pos_t pos;
			while (cur != end) {
				pos = cur->pos;
				out.add(parse(cur, end, error));
				if (error)
					return{};

				if (cur == end) {
					pos.err("Unterminated array");
					error = true;
					return{};
				}

				if (cur->first == JSON_ARRAY_END) {
					++cur;
					break;
				}

				if (cur->first != JSON_COMMA) {
					cur->pos.err("Expecting ','");
					error = true;
					return{};
				}
				++cur;
			}
			return out;
		}

		value dict(tokens_t::const_iterator& cur, tokens_t::const_iterator end, bool& error) {
			map out;
			if (cur != end && cur->first == JSON_OBJECT_END) {
				++cur;
				return out;
			}

			while (cur != end) {
				auto pos = cur->pos;
				auto key = parse(cur, end, error);
				if (error)
					return{};

				if (!key.is<STRING>() || cur == end || cur->first != JSON_COLON) {
					pos.err("Expecting ':'");
					error = true;
					return{};
				}

				pos = cur->pos;
				++cur;

				if (cur == end) {
					pos.err("Unterminated dictionary");
					error = true;
					return{};
				}

				out.add(key.as_string(), parse(cur, end, error));
				if (error)
					return{};

				if (cur == end) {
					pos.err("Unterminated dictionary");
					error = true;
					return{};
				}

				if (cur->first == JSON_OBJECT_END) {
					++cur;
					break;
				}

				if (cur->first != JSON_COMMA) {
					cur->pos.err("Expecting ','");
					error = true;
					return{};
				}
				++cur;
			}
			return out;
		}

	}

	value from_string(const std::string& s) {
		bool error = false;
		parser::input in{s.data(), s.data() + s.length()};
		auto tokens = parser::tokenize(in, error);
		if (error)
			return value{};

		auto begin = std::cbegin(tokens);
		auto ret = parser::parse(begin, std::cend(tokens), error);
		if (error)
			return value{};

		return ret;
	}

	value from_string(const char* data, size_t length)
	{
		bool error = false;
		parser::input in{ data, data + length };
		auto tokens = parser::tokenize(in, error);
		if (error)
			return value{};

		auto begin = std::cbegin(tokens);
		auto ret = parser::parse(begin, std::cend(tokens), error);
		if (error)
			return value{};

		return ret;
	}
};
