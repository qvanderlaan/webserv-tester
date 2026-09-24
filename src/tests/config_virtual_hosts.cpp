#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>
#include <string>

// Helper to assert that a server configuration causes webserv to exit on startup
static void assertConfigFails(ServerInstance& server, const std::string& expectedLogMessage = "")
{
	bool failed = false;
	try
	{
		server.start();
	}
	catch (const std::exception& e)
	{
		failed = true;
		if (!expectedLogMessage.empty())
		{
			TEST_ASSERT_CONTAINS(std::string(e.what()), expectedLogMessage);
		}
	}
	TEST_ASSERT(failed);
}

TEST_CASE("Config VHost: Multiple servers sharing the same host:port with distinct server_names")
{
	int port = 8091;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			server_name site1.local;
			location / {{
				methods GET;
				root ./site1;
				index index.html;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			server_name site2.local;
			location / {{
				methods GET;
				root ./site2;
				index index.html;
			}}
		}}
	)",
									 port, port);

	Sandbox sb;
	sb.writeFile("site1/index.html", "SITE_1_PAGE");
	sb.writeFile("site2/index.html", "SITE_2_PAGE");
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	server.start();

	HttpClient client("127.0.0.1", port);
	TEST_ASSERT(client.waitForServer());

	HttpResponse res1 = client.get("/", {{"Host", "site1.local"}});
	TEST_ASSERT_EQ(res1.statusCode, 200);
	TEST_ASSERT_CONTAINS(res1.body, "SITE_1_PAGE");

	HttpResponse res2 = client.get("/", {{"Host", "site2.local"}});
	TEST_ASSERT_EQ(res2.statusCode, 200);
	TEST_ASSERT_CONTAINS(res2.body, "SITE_2_PAGE");
}

TEST_CASE("Config VHost: One default/catch-all server and one named server on same host:port")
{
	int port = 8092;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			location / {{
				methods GET;
				root ./default_site;
				index index.html;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			server_name specific.local;
			location / {{
				methods GET;
				root ./specific_site;
				index index.html;
			}}
		}}
	)",
									 port, port);

	Sandbox sb;
	sb.writeFile("default_site/index.html", "DEFAULT_CATCH_ALL");
	sb.writeFile("specific_site/index.html", "SPECIFIC_SITE");
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	server.start();

	HttpClient client("127.0.0.1", port);
	TEST_ASSERT(client.waitForServer());

	HttpResponse resDefault = client.get("/", {{"Host", "random.domain.com"}});
	TEST_ASSERT_EQ(resDefault.statusCode, 200);
	TEST_ASSERT_CONTAINS(resDefault.body, "DEFAULT_CATCH_ALL");

	HttpResponse resSpecific = client.get("/", {{"Host", "specific.local"}});
	TEST_ASSERT_EQ(resSpecific.statusCode, 200);
	TEST_ASSERT_CONTAINS(resSpecific.body, "SPECIFIC_SITE");
}

TEST_CASE("Config VHost: Same server_name allowed on DIFFERENT ports")
{
	int port1 = 8093;
	int port2 = 8094;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			server_name shared-domain.com;
			location / {{
				methods GET;
				root ./site1;
				index index.html;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			server_name shared-domain.com;
			location / {{
				methods GET;
				root ./site2;
				index index.html;
			}}
		}}
	)",
									 port1, port2);

	Sandbox sb;
	sb.writeFile("site1/index.html", "PORT_1_CONTENT");
	sb.writeFile("site2/index.html", "PORT_2_CONTENT");
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port1);
	server.start();

	HttpClient client1("127.0.0.1", port1);
	HttpClient client2("127.0.0.1", port2);

	TEST_ASSERT(client1.waitForServer());
	TEST_ASSERT(client2.waitForServer());

	HttpResponse res1 = client1.get("/", {{"Host", "shared-domain.com"}});
	TEST_ASSERT_EQ(res1.statusCode, 200);
	TEST_ASSERT_CONTAINS(res1.body, "PORT_1_CONTENT");

	HttpResponse res2 = client2.get("/", {{"Host", "shared-domain.com"}});
	TEST_ASSERT_EQ(res2.statusCode, 200);
	TEST_ASSERT_CONTAINS(res2.body, "PORT_2_CONTENT");
}

TEST_CASE("Config VHost Error: Duplicate listen directive within the same server block")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			listen 127.0.0.1:{PORT};
			location / {
				methods GET;
				root ./;
			}
		}
	)");

	assertConfigFails(server, "Duplicate listen");
}

TEST_CASE("Config VHost Error: Duplicate server_name on the same host:port")
{
	int port = 8095;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			server_name example.com;
			location / {{
				methods GET;
				root ./site1;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			server_name example.com;
			location / {{
				methods GET;
				root ./site2;
			}}
		}}
	)",
									 port, port);

	Sandbox sb;
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	assertConfigFails(server, "Conflicting server_name");
}

TEST_CASE("Config VHost Error: Duplicate case-insensitive server_name on same host:port")
{
	int port = 8096;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			server_name mysite.org;
			location / {{
				methods GET;
				root ./site1;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			server_name MYSITE.ORG;
			location / {{
				methods GET;
				root ./site2;
			}}
		}}
	)",
									 port, port);

	Sandbox sb;
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	assertConfigFails(server, "Conflicting server_name");
}

TEST_CASE("Config VHost Error: Multiple default (unnamed) servers on same host:port")
{
	int port = 8097;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			location / {{
				methods GET;
				root ./site1;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			location / {{
				methods GET;
				root ./site2;
			}}
		}}
	)",
									 port, port);

	Sandbox sb;
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	assertConfigFails(server, "Conflicting default");
}
