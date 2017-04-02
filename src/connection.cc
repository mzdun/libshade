#include "shade/connection.h"

namespace shade {
	connection connection::logged(const std::string& username) const
	{
		return { browser_, base_, id_, root_ + "/" + username };
	}

	connection connection::unlogged() const
	{
		return { browser_, base_, id_ };
	}

	bool connection::get(const std::string& resource, http::client_ptr client) const
	{
		return browser_->get(uri_with(resource), std::move(client));
	}

	bool connection::del(const std::string& resource, http::client_ptr client) const
	{
		return browser_->del(uri_with(resource), std::move(client));
	}

	bool connection::put(const std::string& resource, const std::string& data, http::client_ptr client) const
	{
		return browser_->put(uri_with(resource), data, std::move(client));
	}

	bool connection::post(const std::string& resource, const std::string& data, http::client_ptr client) const
	{
		return browser_->post(uri_with(resource), data, std::move(client));
	}
}
