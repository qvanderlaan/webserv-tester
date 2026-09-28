#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>

TEST_CASE("Virtual host matching is case-insensitive (RFC 9110)")
{
	int port = 8097;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			server_name mydomain.org;
			location / {{
				methods GET;
				root ./site;
				index index.html;
			}}
		}}
	)",
									 port);

	Sandbox sb;
	sb.writeFile("site/index.html", "MYDOMAIN_CONTENT");
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	server.start();

	HttpClient client("127.0.0.1", port);
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/", {{"Host", "MYDOMAIN.ORG"}});
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "MYDOMAIN_CONTENT");
}

TEST_CASE("Virtual host matching strips port number from Host header")
{
	int port = 8098;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			server_name target.local;
			location / {{
				methods GET;
				root ./target_site;
				index index.html;
			}}
		}}
	)",
									 port);

	Sandbox sb;
	sb.writeFile("target_site/index.html", "PORT_STRIPPED_MATCH_OK");
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	server.start();

	HttpClient client("127.0.0.1", port);
	TEST_ASSERT(client.waitForServer());

	std::string hostWithPort = std::format("target.local:{}", port);
	HttpResponse res = client.get("/", {{"Host", hostWithPort}});

	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "PORT_STRIPPED_MATCH_OK");
}
