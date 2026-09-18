#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <arpa/inet.h>
#include <format>
#include <sys/socket.h>
#include <unistd.h>

TEST_CASE("Lowercase and mixed-case header names are parsed correctly (RFC 9110)")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET POST;
				root ./;
			}
		}
	)");
	server.sandbox().writeFile("hello.txt", "Lowercase Header OK");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Lowercase 'host' and 'connection'
	std::string rawReq =
		std::format("GET /hello.txt HTTP/1.1\r\nhost: 127.0.0.1:{}\r\nconnection: close\r\n\r\n", server.getPort());
	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "Lowercase Header OK");

	// Mixed-case headers
	std::string rawReqMixed =
		std::format("GET /hello.txt HTTP/1.1\r\nHoSt: 127.0.0.1:{}\r\nCoNnEcTiOn: close\r\n\r\n", server.getPort());
	HttpResponse resMixed = client.sendRaw(rawReqMixed);
	TEST_ASSERT_EQ(resMixed.statusCode, 200);
}

TEST_CASE("HEAD method returns Content-Length without body and preserves Keep-Alive")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET HEAD;
				root ./;
			}
		}
	)");
	std::string content = "This is static file content for HEAD testing.";
	server.sandbox().writeFile("test.txt", content);
	server.start();

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server.getPort());
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	TEST_ASSERT(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

	// Send HEAD followed by GET over keep-alive
	std::string req1 = std::format(
		"HEAD /test.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: keep-alive\r\n\r\n", server.getPort());
	::send(sock, req1.data(), req1.size(), 0);

	char buf[2048];
	ssize_t n1 = recv(sock, buf, sizeof(buf) - 1, 0);
	TEST_ASSERT(n1 > 0);
	buf[n1] = '\0';
	std::string rawHeadRes(buf, n1);

	HttpResponse headParsed = HttpResponse::parse(rawHeadRes);
	TEST_ASSERT_EQ(headParsed.statusCode, 200);
	TEST_ASSERT_EQ(headParsed.getHeaders("Content-Length"), std::to_string(content.size()));
	TEST_ASSERT_EQ((int)headParsed.body.size(), 0); // Strictly no body bytes!

	// Second request on same connection
	std::string req2 =
		std::format("GET /test.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: close\r\n\r\n", server.getPort());
	::send(sock, req2.data(), req2.size(), 0);

	std::string rawGetRes;
	while (true)
	{
		ssize_t n2 = recv(sock, buf, sizeof(buf), 0);
		if (n2 <= 0)
			break;
		rawGetRes.append(buf, n2);
	}
	close(sock);

	HttpResponse getParsed = HttpResponse::parse(rawGetRes);
	TEST_ASSERT_EQ(getParsed.statusCode, 200);
	TEST_ASSERT_CONTAINS(getParsed.body, content);
}

TEST_CASE("Half-close (shutdown SHUT_WR) still receives full response")
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
	server.sandbox().writeFile("half_close.txt", "RESPONSE_AFTER_HALF_CLOSE");
	server.start();

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server.getPort());
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	TEST_ASSERT(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

	std::string req = std::format(
		"GET /half_close.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: close\r\n\r\n", server.getPort());
	::send(sock, req.data(), req.size(), 0);

	// Client signals it finished sending
	TEST_ASSERT(shutdown(sock, SHUT_WR) == 0);

	std::string response;
	char buf[1024];
	while (true)
	{
		ssize_t n = recv(sock, buf, sizeof(buf), 0);
		if (n <= 0)
			break;
		response.append(buf, n);
	}
	close(sock);

	HttpResponse parsed = HttpResponse::parse(response);
	TEST_ASSERT_EQ(parsed.statusCode, 200);
	TEST_ASSERT_CONTAINS(parsed.body, "RESPONSE_AFTER_HALF_CLOSE");
}

TEST_CASE("Keep-Alive clears headers between requests for virtual host routing")
{
	int port = 8095;
	std::string config = std::format(R"(
		server {{
			listen 127.0.0.1:{};
			server_name site-a.com;
			location / {{
				methods GET;
				root ./site_a;
				index index.html;
			}}
		}}
		server {{
			listen 127.0.0.1:{};
			server_name site-b.com;
			location / {{
				methods GET;
				root ./site_b;
				index index.html;
			}}
		}}
	)",
									 port, port);

	Sandbox sb;
	sb.writeFile("site_a/index.html", "SITE_A_CONTENT");
	sb.writeFile("site_b/index.html", "SITE_B_CONTENT");
	sb.writeConfig(config);

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.conf", port);
	server.start();

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	TEST_ASSERT(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

	// Request 1: site-a.com
	std::string req1 = std::format("GET / HTTP/1.1\r\nHost: site-a.com:{}\r\nConnection: keep-alive\r\n\r\n", port);
	::send(sock, req1.data(), req1.size(), 0);

	char buf[2048];
	ssize_t n1 = recv(sock, buf, sizeof(buf) - 1, 0);
	TEST_ASSERT(n1 > 0);
	buf[n1] = '\0';
	TEST_ASSERT_CONTAINS(std::string(buf), "SITE_A_CONTENT");

	// Request 2 on same socket: site-b.com (should not retain site-a.com Host)
	std::string req2 = std::format("GET / HTTP/1.1\r\nHost: site-b.com:{}\r\nConnection: close\r\n\r\n", port);
	::send(sock, req2.data(), req2.size(), 0);

	std::string res2;
	while (true)
	{
		ssize_t n2 = recv(sock, buf, sizeof(buf), 0);
		if (n2 <= 0)
			break;
		res2.append(buf, n2);
	}
	close(sock);

	TEST_ASSERT_CONTAINS(res2, "SITE_B_CONTENT");
}

TEST_CASE("HEAD method is implied by GET in location configuration")
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
	server.sandbox().writeFile("index.html", "Implied HEAD test");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	std::string rawReq =
		std::format("HEAD /index.html HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: close\r\n\r\n", server.getPort());
	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 200);
}