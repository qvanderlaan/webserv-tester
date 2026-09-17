#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>

TEST_CASE("POST with Transfer-Encoding chunked decodes body properly")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods POST;
				root ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Construct raw HTTP chunked request:
	// "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n" -> "Wikipedia"
	std::string rawReq = std::format("POST /test_chunk.txt HTTP/1.1\r\n"
									 "Host: 127.0.0.1:{}\r\n"
									 "Transfer-Encoding: chunked\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "4\r\nWiki\r\n"
									 "5\r\npedia\r\n"
									 "0\r\n\r\n",
									 server.getPort());

	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT(res.statusCode == 200 || res.statusCode == 201);
}