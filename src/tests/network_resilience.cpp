#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <arpa/inet.h>
#include <format>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

TEST_CASE("Slow client (drip-feed request) does not stall or crash server")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET;
				root ./;
				index index.html;
			}
		}
	)");
	server.sandbox().writeFile("index.html", "Slowloris Resisted");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Open raw socket and send request byte-by-byte with small delays
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT(sock >= 0);

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server.getPort());
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	TEST_ASSERT(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

	std::string req =
		std::format("GET / HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: close\r\n\r\n", server.getPort());
	for (char c : req)
	{
		::send(sock, &c, 1, 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

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
	TEST_ASSERT_CONTAINS(parsed.body, "Slowloris Resisted");
}

TEST_CASE("Abrupt client disconnection does not crash server with SIGPIPE")
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
	server.sandbox().writeFile("large.txt", std::string(100000, 'X'));
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Connect, send partial request, and immediately close socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server.getPort());
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0)
	{
		std::string partial = "GET /large.txt HTTP/1.1\r\n";
		::send(sock, partial.data(), partial.size(), 0);
		close(sock); // Abrupt RST / FIN
	}

	// Ensure the server survived the abrupt close and handles subsequent clients
	HttpResponse res = client.get("/large.txt");
	TEST_ASSERT_EQ(res.statusCode, 200);
}

TEST_CASE("Keep-Alive allows multiple sequential requests on same socket")
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
	server.sandbox().writeFile("one.txt", "Response 1");
	server.sandbox().writeFile("two.txt", "Response 2");
	server.start();

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server.getPort());
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	TEST_ASSERT(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

	// First Request
	std::string req1 =
		std::format("GET /one.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: keep-alive\r\n\r\n", server.getPort());
	::send(sock, req1.data(), req1.size(), 0);

	char buf[2048];
	ssize_t n1 = recv(sock, buf, sizeof(buf) - 1, 0);
	TEST_ASSERT(n1 > 0);
	buf[n1] = '\0';
	TEST_ASSERT_CONTAINS(std::string(buf), "Response 1");

	// Second Request over the same open socket
	std::string req2 =
		std::format("GET /two.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: close\r\n\r\n", server.getPort());
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

	TEST_ASSERT_CONTAINS(res2, "Response 2");
}
