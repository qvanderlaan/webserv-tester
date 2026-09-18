#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <iostream>
#include <sys/stat.h>

TEST_CASE("CGI GET parses QUERY_STRING and outputs body")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /cgi-bin {
				methods GET POST;
				root ./;
				cgi .py /usr/bin/python3;
			}
		}
	)");

	std::string script = "#!/usr/bin/env python3\n"
						 "import os\n"
						 "query = os.environ.get('QUERY_STRING', '')\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print(f'QUERY_RECEIVED:{query}')\n";

	server.sandbox().writeFile("cgi-bin/test.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/test.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/test.py?user=42student&role=tester");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "QUERY_RECEIVED:user=42student&role=tester");
}

TEST_CASE("CGI POST reads body from standard input")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			client_max_body_size 5000;
			location /cgi-bin {
				methods GET POST;
				root ./;
				cgi .py /usr/bin/python3;
			}
		}
	)");

	std::string script = "#!/usr/bin/env python3\n"
						 "import sys\n"
						 "body = sys.stdin.read()\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print(f'BODY_ECHO:{body}')\n";

	server.sandbox().writeFile("cgi-bin/echo.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/echo.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.post("/cgi-bin/echo.py", "CustomPayloadForCGI");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "BODY_ECHO:CustomPayloadForCGI");
}

TEST_CASE("CGI crash or non-zero exit returns 500/502 without crashing server")
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

	std::string script = "#!/usr/bin/env python3\n"
						 "import sys\n"
						 "sys.exit(1)\n";

	server.sandbox().writeFile("cgi-bin/crash.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/crash.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/crash.py");
	TEST_ASSERT(res.statusCode == 500 || res.statusCode == 502);

	// Verify server remains alive for subsequent requests
	server.sandbox().writeFile("index.html", "OK");
	HttpResponse okRes = client.get("/index.html");
	TEST_ASSERT_EQ(okRes.statusCode, 200);
}
