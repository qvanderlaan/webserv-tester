#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <format>
#include <string>
#include <sys/stat.h>

// Helper to assert that a server configuration causes webserv to exit on startup
static void assertConfigFails(ServerInstance& server, const std::string& expectedLogMessage = "")
{
	bool failed = false;
	try
	{
		server.start();
	}
	catch (const std::exception& e)
	{
		failed = true;
		if (!expectedLogMessage.empty())
		{
			TEST_ASSERT_CONTAINS(std::string(e.what()), expectedLogMessage);
		}
	}
	TEST_ASSERT(failed);
}

TEST_CASE("Config: Minimal valid server configuration starts successfully")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				root ./;
			}
		}
	)");

	server.sandbox().writeFile("index.html", "MINIMAL_OK");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/index.html");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "MINIMAL_OK");
}

TEST_CASE("Config: Multiple HTTP methods declared on single line")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				methods GET POST DELETE;
				root ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/nonexistent.txt");
	TEST_ASSERT_EQ(res.statusCode, 404);
}

TEST_CASE("Config: 'return' directive functions identically to 'redirect'")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /old {
				methods GET;
				return 301 /new;
			}
			location /new {
				methods GET;
				root ./;
				index index.html;
			}
		}
	)");

	server.sandbox().writeFile("index.html", "NEW_TARGET");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/old");
	TEST_ASSERT_EQ(res.statusCode, 301);
	TEST_ASSERT_EQ(res.getHeaders("Location"), "/new");
}

TEST_CASE("Config: Multiple error_page directives for different status codes")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			error_page 404 /custom_404.html;
			error_page 403 /custom_403.html;
			location / {
				methods GET;
				root ./;
			}
		}
	)");

	server.sandbox().writeFile("custom_404.html", "ERROR_404_PAGE");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/missing.html");
	TEST_ASSERT_EQ(res.statusCode, 404);
	TEST_ASSERT_CONTAINS(res.body, "ERROR_404_PAGE");
}

TEST_CASE("Config: Multiple CGI interpreters for different file extensions")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /cgi-bin {
				methods GET;
				root ./;
				cgi .py /usr/bin/python3;
				cgi .sh /bin/sh;
			}
		}
	)");

	server.sandbox().writeFile("cgi-bin/test.sh", "#!/bin/sh\nprintf 'Content-Type: text/plain\\r\\n\\r\\nSH_OK\\n'\n");
	chmod((server.sandbox().getPath() / "cgi-bin/test.sh").c_str(), 0755);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/cgi-bin/test.sh");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "SH_OK");
}

TEST_CASE("Config Error: Missing semicolon in server directive")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT}
			location / {
				root ./;
			}
		}
	)");

	assertConfigFails(server, "expected ';'");
}

TEST_CASE("Config Error: Missing semicolon in location directive")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				root ./
			}
		}
	)");

	assertConfigFails(server, "expected ';'");
}

TEST_CASE("Config Error: Port number exceeds 65535")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:70000;
			location / {
				root ./;
			}
		}
	)");

	assertConfigFails(server, "invalid port value");
}

TEST_CASE("Config Error: Port number is zero")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:0;
			location / {
				root ./;
			}
		}
	)");

	assertConfigFails(server, "invalid port value");
}

TEST_CASE("Config Error: Missing port separator colon in listen directive")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1;
			location / {
				root ./;
			}
		}
	)");

	assertConfigFails(server, "expected value");
}

TEST_CASE("Config Error: Unknown directive in server block")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			unsupported_server_directive on;
			location / {
				root ./;
			}
		}
	)");

	assertConfigFails(server, "unknown directive");
}

TEST_CASE("Config Error: Unknown directive in location block")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				proxy_pass http://localhost:8080;
			}
		}
	)");

	assertConfigFails(server, "unexpected token");
}

TEST_CASE("Config Error: Invalid autoindex value (expected 'on' or 'off')")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				autoindex enabled;
			}
		}
	)");

	assertConfigFails(server, "expected: 'on' or 'off'");
}

TEST_CASE("Config Error: Unclosed server block brace (unexpected EOF)")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				root ./;
			}
	)");

	assertConfigFails(server, "unexpected end of file");
}

TEST_CASE("Config Error: Unclosed location block brace (unexpected EOF)")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location / {
				root ./;
		}
	)");

	assertConfigFails(server, "unexpected end of file");
}

TEST_CASE("Config Error: error_page missing target path argument")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			error_page 404;
			location / {
				root ./;
			}
		}
	)");

	assertConfigFails(server, "expected value");
}

TEST_CASE("Config Error: Empty config file rejected")
{
	Sandbox sb;
	sb.writeFile("empty.conf", "");

	ServerInstance server(ctx.webservBin, std::move(sb), "empty.conf", 8080);
	assertConfigFails(server, "config file is empty");
}

TEST_CASE("Config Error: File without .conf extension is rejected")
{
	Sandbox sb;
	sb.writeFile("webserv.txt", "server { listen 127.0.0.1:8080; location / { root ./; } }");

	ServerInstance server(ctx.webservBin, std::move(sb), "webserv.txt", 8080);
	assertConfigFails(server, "must have .conf extension");
}
