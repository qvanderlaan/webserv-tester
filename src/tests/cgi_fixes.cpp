#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <format>
#include <future>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

TEST_CASE("Pipelining second request while CGI is executing does not freeze server")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /cgi-bin {
				methods GET;
				root ./;
				cgi .py /usr/bin/python3;
			}
			location / {
				methods GET;
				root ./;
			}
		}
	)");

	// Slow script that takes 1 second
	std::string script = "#!/usr/bin/env python3\n"
						 "import time\n"
						 "time.sleep(1)\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\nCGI_DONE', end='')\n";

	server.sandbox().writeFile("cgi-bin/slow.py", script);
	server.sandbox().writeFile("hello.txt", "HELLO_STATIC");
	chmod((server.sandbox().getPath() / "cgi-bin/slow.py").c_str(), 0755);
	server.start();

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT(sock >= 0);

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server.getPort());
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	TEST_ASSERT(connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);

	// Pipeline: Send CGI request and immediately send static file request
	std::string pipelinedReq =
		std::format("GET /cgi-bin/slow.py HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: keep-alive\r\n\r\n"
					"GET /hello.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nConnection: close\r\n\r\n",
					server.getPort(), server.getPort());

	ssize_t sent = ::send(sock, pipelinedReq.data(), pipelinedReq.size(), 0);
	TEST_ASSERT(sent == static_cast<ssize_t>(pipelinedReq.size()));

	std::string response;
	char buf[4096];
	while (true)
	{
		ssize_t n = recv(sock, buf, sizeof(buf), 0);
		if (n <= 0)
			break;
		response.append(buf, n);
	}
	close(sock);

	TEST_ASSERT_CONTAINS(response, "CGI_DONE");
	TEST_ASSERT_CONTAINS(response, "HELLO_STATIC");
}

TEST_CASE("CGI script printing and sleeping does not block event loop")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /cgi-bin {
				methods GET;
				root ./;
				cgi .py /usr/bin/python3;
			}
			location / {
				methods GET;
				root ./;
			}
		}
	)");

	// Prints header/content first, then sleeps (blocking waitpid trap)
	std::string script = "#!/usr/bin/env python3\n"
						 "import time, sys\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\nEARLY_OUTPUT', flush=True)\n"
						 "time.sleep(2)\n";

	server.sandbox().writeFile("cgi-bin/sleepy.py", script);
	server.sandbox().writeFile("fast.txt", "FAST_OK");
	chmod((server.sandbox().getPath() / "cgi-bin/sleepy.py").c_str(), 0755);
	server.start();

	HttpClient client1 = server.client();
	HttpClient client2 = server.client();
	TEST_ASSERT(client1.waitForServer());

	// Launch slow CGI asynchronously
	auto cgiFuture = std::async(std::launch::async, [&]() { return client1.get("/cgi-bin/sleepy.py"); });

	// Give CGI time to start & sleep
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	// Server should immediately answer client2 without waiting 2 seconds
	auto start = std::chrono::high_resolution_clock::now();
	HttpResponse fastRes = client2.get("/fast.txt");
	auto durationMs =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
			.count();

	TEST_ASSERT_EQ(fastRes.statusCode, 200);
	TEST_ASSERT_CONTAINS(fastRes.body, "FAST_OK");
	TEST_ASSERT(durationMs < 1000); // Must be handled asynchronously within < 1s

	HttpResponse cgiRes = cgiFuture.get();
	TEST_ASSERT_EQ(cgiRes.statusCode, 200);
	TEST_ASSERT_CONTAINS(cgiRes.body, "EARLY_OUTPUT");
}

TEST_CASE("GET request to CGI script reading stdin gets EOF immediately")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /cgi-bin {
				methods GET;
				root ./;
				cgi .py /usr/bin/python3;
			}
		}
	)");

	// Script tries to read from stdin on a GET request
	std::string script = "#!/usr/bin/env python3\n"
						 "import sys\n"
						 "data = sys.stdin.read()\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print(f'READ_BYTES:{len(data)}')\n";

	server.sandbox().writeFile("cgi-bin/stdin_read.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/stdin_read.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// If stdin write end was kept open by parent, read() hangs until timeout
	auto start = std::chrono::high_resolution_clock::now();
	HttpResponse res = client.get("/cgi-bin/stdin_read.py");
	auto durationMs =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
			.count();

	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "READ_BYTES:0");
	TEST_ASSERT(durationMs < 1500);
}