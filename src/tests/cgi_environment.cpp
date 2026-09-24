#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <sys/stat.h>

TEST_CASE("CGI passes Cookie header as HTTP_COOKIE environment variable")
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
						 "import os\n"
						 "cookie = os.environ.get('HTTP_COOKIE', 'NONE')\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print(f'COOKIE_VAL:{cookie}')\n";

	server.sandbox().writeFile("cgi-bin/cookie_test.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/cookie_test.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/cookie_test.py", {{"Cookie", "session_token=secret123; user_id=42"}});

	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "COOKIE_VAL:session_token=secret123; user_id=42");
}

TEST_CASE("CGI sets CONTENT_TYPE and CONTENT_LENGTH environment variables on POST")
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
						 "import os\n"
						 "ctype = os.environ.get('CONTENT_TYPE', '')\n"
						 "clen = os.environ.get('CONTENT_LENGTH', '')\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print(f'CTYPE:{ctype}|CLEN:{clen}')\n";

	server.sandbox().writeFile("cgi-bin/env_meta.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/env_meta.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	std::string payload = "{\"key\": \"value\"}";
	HttpResponse res = client.post("/cgi-bin/env_meta.py", payload, {{"Content-Type", "application/json"}});

	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "CTYPE:application/json|CLEN:" + std::to_string(payload.size()));
}

TEST_CASE("CGI response Status header overrides default 200 status code")
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
						 "print('Status: 418 I am a teapot\\r\\n', end='')\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print('Short and stout')\n";

	server.sandbox().writeFile("cgi-bin/teapot.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/teapot.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/teapot.py");
	TEST_ASSERT_EQ(res.statusCode, 418);
	TEST_ASSERT_CONTAINS(res.body, "Short and stout");
}

TEST_CASE("CGI Set-Cookie and custom response headers are preserved in HTTP response")
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
						 "print('Set-Cookie: user_auth=abcdef12345; Path=/; HttpOnly\\r\\n', end='')\n"
						 "print('X-Custom-Server: WebservCGI\\r\\n', end='')\n"
						 "print('Content-Type: text/plain\\r\\n\\r\\n', end='')\n"
						 "print('Cookie attached')\n";

	server.sandbox().writeFile("cgi-bin/set_cookie.py", script);
	chmod((server.sandbox().getPath() / "cgi-bin/set_cookie.py").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/set_cookie.py");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.getHeaders("Set-Cookie"), "user_auth=abcdef12345");
	TEST_ASSERT_CONTAINS(res.getHeaders("X-Custom-Server"), "WebservCGI");
}
