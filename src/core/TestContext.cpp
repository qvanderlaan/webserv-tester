#include "TestContext.hpp"

ServerInstance TestContext::spawnServer(const std::string& configTemplate, int customPort)
{
	Sandbox sb;
	ServerInstance server(webservBin, std::move(sb), "webserv.conf", customPort);

	std::string conf = configTemplate;
	std::string placeholder = "{PORT}";
	size_t pos = 0;
	while ((pos = conf.find(placeholder, pos)) != std::string::npos)
	{
		conf.replace(pos, placeholder.length(), std::to_string(server.getPort()));
		pos += std::to_string(server.getPort()).length();
	}

	server.sandbox().writeConfig(conf);
	return (server);
}
