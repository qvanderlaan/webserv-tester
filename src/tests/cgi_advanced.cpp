#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>
#include <iostream>
#include <sys/stat.h>

TEST_CASE("CGI executes in script directory for relative file access")
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

	// CGI opens a relative file in its own directory: open('./local_data.txt')
	std::string script = "#!/usr/bin/env python3\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "with open('local_data.txt', 'r') as f:\n"
						 "    print(f.read(), end='')\n";

	server.sandbox().writeFile("cgi-bin/local_data.txt", "RELATIVE_FILE_CONTENT_OK");
	server.sandbox().writeFile("cgi-bin/rel_test.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/rel_test.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/rel_test.py");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "RELATIVE_FILE_CONTENT_OK");
}

TEST_CASE("CGI infinite loop times out (504/500) and does not hang the server")
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
						 "import time\n"
						 "while True:\n"
						 "    time.sleep(1)\n";

	server.sandbox().writeFile("cgi-bin/infinite.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/infinite.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/infinite.py");

	// Should terminate with Gateway Timeout or Internal Error
	TEST_ASSERT(res.statusCode == 504 || res.statusCode == 500 || res.statusCode == 502);
}

TEST_CASE("CGI un-chunks chunked POST request body before feeding stdin")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /cgi-bin {
				methods POST;
				root ./;
				cgi .py /usr/bin/python3;
			}
		}
	)");

	std::string script = "#!/usr/bin/env python3\n"
						 "import sys\n"
						 "data = sys.stdin.read()\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print(f'PARSED_LEN:{len(data)}:{data}', end='')\n";

	server.sandbox().writeFile("cgi-bin/chunked_cgi.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/chunked_cgi.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Chunked payload for "HelloWorld"
	std::string rawReq = std::format("POST /cgi-bin/chunked_cgi.py HTTP/1.1\r\n"
									 "Host: 127.0.0.1:{}\r\n"
									 "Transfer-Encoding: chunked\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "5\r\nHello\r\n"
									 "5\r\nWorld\r\n"
									 "0\r\n\r\n",
									 server.getPort());

	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "PARSED_LEN:10:HelloWorld");
}
