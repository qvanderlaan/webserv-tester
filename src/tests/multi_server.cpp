#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>

TEST_CASE("Server listens on multiple ports delivering different content")
{
	int port1 = 8091;
	int port2 = 8092;

	// Use custom template with two port slots
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			location / {{
				methods GET;
				root ./site1;
				index index.html;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			location / {{
				methods GET;
				root ./site2;
				index index.html;
			}}
		}}
	)",
									 port1, port2);

	Sandbox sb;
	sb.writeFile("site1/index.html", "<h1>Content from Site 1</h1>");
	sb.writeFile("site2/index.html", "<h1>Content from Site 2</h1>");
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port1);
	server.start();

	HttpClient client1("127.0.0.1", port1);
	HttpClient client2("127.0.0.1", port2);

	TEST_ASSERT(client1.waitForServer());
	TEST_ASSERT(client2.waitForServer());

	HttpResponse res1 = client1.get("/");
	TEST_ASSERT_EQ(res1.statusCode, 200);
	TEST_ASSERT_CONTAINS(res1.body, "Content from Site 1");

	HttpResponse res2 = client2.get("/");
	TEST_ASSERT_EQ(res2.statusCode, 200);
	TEST_ASSERT_CONTAINS(res2.body, "Content from Site 2");
}