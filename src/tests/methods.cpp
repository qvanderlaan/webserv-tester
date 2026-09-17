#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"

TEST_CASE("POST /upload writes file or processes data")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			client_max_body_size 5000;
			location / {
				methods GET POST;
				root ./;
				upload_store ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	std::string payload = "Hello POST payload!";
	HttpResponse res = client.post("/test.txt", payload, {{"Content-Type", "text/plain"}});

	// Should be 200 OK or 201 Created
	TEST_ASSERT(res.statusCode == 200 || res.statusCode == 201);
}

TEST_CASE("DELETE existing file removes it and returns 200/204")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET DELETE;
				root ./;
			}
		}
	)");

	server.sandbox().writeFile("delete_me.txt", "temporary content");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.deleteReq("/delete_me.txt");
	TEST_ASSERT(res.statusCode == 200 || res.statusCode == 204);

	// Confirm the file is now gone
	HttpResponse getRes = client.get("/delete_me.txt");
	TEST_ASSERT_EQ(getRes.statusCode, 404);
}

TEST_CASE("DELETE nonexistent file returns 404")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET DELETE;
				root ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.deleteReq("/nonexistent_file.txt");
	TEST_ASSERT_EQ(res.statusCode, 404);
}

TEST_CASE("Method not allowed in location returns 405")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /readonly {
				methods GET;
				root ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.deleteReq("/readonly/some_file.txt");
	TEST_ASSERT_EQ(res.statusCode, 405);
}

TEST_CASE("Unknown HTTP method returns error and does not crash")
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

	std::string rawReq = "INVALIDMETHOD / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	HttpResponse res = client.sendRaw(rawReq);
	// Should reject with 400, 405, or 501 Not Implemented, but not crash
	TEST_ASSERT(res.statusCode == 400 || res.statusCode == 405 || res.statusCode == 501);
}
