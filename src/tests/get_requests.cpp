#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"

TEST_CASE("GET / returns 200 with index.html")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET;
				root ./;
				index hello.html;
			}
		}
	)");

	server.sandbox().writeFile("hello.html", "<h1>Hello World!</h1>");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "<h1>Hello World!</h1>");
}

TEST_CASE("GET nonexistent file returns 404")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET;
				root ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/missing.html");
	TEST_ASSERT_EQ(res.statusCode, 404);
}