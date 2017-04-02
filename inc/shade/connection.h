#pragma once

#include "shade/http.h"

namespace shade {
	class connection {
		http* browser_;
		tangle::uri base_;
		std::string id_;
		std::string root_ = "/api";

		tangle::uri uri_with(const std::string& resource) const {
			return tangle::uri{ base_ }.path(root_ + resource);
		}
	public:
		connection() = default;
		connection(http* browser, const tangle::uri& base, const std::string& id, const std::string& root = "/api")
			: browser_{ browser }
			, base_{ base }
			, id_{ id }
			, root_{ root }
		{
		}
		const std::string& id() const { return id_; }

		connection logged(const std::string& username) const;
		connection unlogged() const;

		bool get(const std::string& resource, http::client_ptr client) const;
		bool del(const std::string& resource, http::client_ptr client) const;
		bool put(const std::string& resource, const std::string& data, http::client_ptr client) const;
		bool post(const std::string& resource, const std::string& data, http::client_ptr client) const;
	};
}
