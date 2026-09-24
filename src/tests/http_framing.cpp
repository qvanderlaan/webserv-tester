#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>

TEST_CASE("RFC 9112: Both Transfer-Encoding and Content-Length is rejected with 400 (Smuggling)")
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

	std::string rawReq = std::format("POST /smuggle HTTP/1.1\r\n"
									 "Host: 127.0.0.1:{}\r\n"
									 "Transfer-Encoding: chunked\r\n"
									 "Content-Length: 6\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "0\r\n\r\n",
									 server.getPort());

	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 400);
}

TEST_CASE("Conflicting duplicate Content-Length headers return 400 Bad Request")
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

	std::string rawReq = std::format("POST /conflict HTTP/1.1\r\n"
									 "Host: 127.0.0.1:{}\r\n"
									 "Content-Length: 5\r\n"
									 "Content-Length: 10\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "hello",
									 server.getPort());

	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 400);
}

TEST_CASE("Non-numeric or negative Content-Length returns 400 Bad Request")
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

	std::string reqNeg = std::format("POST /data HTTP/1.1\r\n"
									 "Host: 127.0.0.1:{}\r\n"
									 "Content-Length: -15\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "hello",
									 server.getPort());
	HttpResponse resNeg = client.sendRaw(reqNeg);
	TEST_ASSERT_EQ(resNeg.statusCode, 400);

	std::string reqAlpha = std::format("POST /data HTTP/1.1\r\n"
									   "Host: 127.0.0.1:{}\r\n"
									   "Content-Length: ten\r\n"
									   "Connection: close\r\n"
									   "\r\n"
									   "hello",
									   server.getPort());
	HttpResponse resAlpha = client.sendRaw(reqAlpha);
	TEST_ASSERT_EQ(resAlpha.statusCode, 400);
}

TEST_CASE("Chunked body exceeding client_max_body_size returns 413 Payload Too Large")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			client_max_body_size 15;
			location / {
				methods POST;
				root ./;
				upload_store ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	std::string rawReq = std::format("POST /chunk_overflow.txt HTTP/1.1\r\n"
									 "Host: 127.0.0.1:{}\r\n"
									 "Transfer-Encoding: chunked\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "A\r\n0123456789\r\n"
									 "A\r\n0123456789\r\n"
									 "0\r\n\r\n",
									 server.getPort());

	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 413);
}

TEST_CASE("Chunk extensions and trailing headers are handled cleanly")
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

	std::string rawReq = std::format("POST /test_ext.txt HTTP/1.1\r\n"
									 "Host: 127.0.0.1:{}\r\n"
									 "Transfer-Encoding: chunked\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "5;token=valid\r\nHello\r\n"
									 "6;another=param\r\n World\r\n"
									 "0\r\n"
									 "X-Trailer-Checksum: abc123\r\n"
									 "\r\n",
									 server.getPort());

	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT(res.statusCode == 200 || res.statusCode == 201);
}
