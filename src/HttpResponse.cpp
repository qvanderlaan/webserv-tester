#include "HttpResponse.hpp"
#include <strings.h>

[[nodiscard]]
std::string HttpResponse::getHeaders(std::string& key) const noexcept
{
	for (const auto& [k, v] : headers)
	{
		if (strcasecmp(k.c_str(), key.c_str()) == 0)
			return (v);
	}
	return ("");
}
