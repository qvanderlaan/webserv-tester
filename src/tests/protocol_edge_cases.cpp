#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>

TEST_CASE("HTTP/1.1 request without Host header returns 400 Bad Request")
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

	// RFC 7230: HTTP/1.1 requests without Host header MUST be rejected with 400
	std::string rawReq = "GET / HTTP/1.1\r\nConnection: close\r\n\r\n";
	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 400);
}

TEST_CASE("Path traversal attempt (/../) does not escape sandbox root")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /public {
				methods GET;
				root ./public;
			}
		}
	)");
	server.sandbox().writeFile("secret.txt", "TOP_SECRET_PASSWORD");
	server.sandbox().writeFile("public/index.html", "Public Area");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Attempt directory traversal out of /public to reach secret.txt
	HttpResponse res = client.get("/public/../secret.txt");
	TEST_ASSERT(res.statusCode == 400 || res.statusCode == 403 || res.statusCode == 404);
	TEST_ASSERT(res.body.find("TOP_SECRET_PASSWORD") == std::string::npos);
}

TEST_CASE("Requesting directory without trailing slash redirects with 301")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /folder {
				methods GET;
				root ./;
				index index.html;
			}
		}
	)");
	server.sandbox().writeFile("folder/index.html", "Inside Folder");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Standard web servers redirect /folder to /folder/
	HttpResponse res = client.get("/folder");
	if (res.statusCode == 301 || res.statusCode == 302)
	{
		std::string loc = res.getHeaders("Location");
		TEST_ASSERT(loc.ends_with("/folder/"));
	}
	else
	{
		// If served directly without redirect, body must match index
		TEST_ASSERT_EQ(res.statusCode, 200);
		TEST_ASSERT_CONTAINS(res.body, "Inside Folder");
	}
}

TEST_CASE("URL percent-decoding handles encoded spaces and special characters")
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
	server.sandbox().writeFile("hello world.html", "<h1>Decoded Space</h1>");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/hello%20world.html");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "<h1>Decoded Space</h1>");
}