#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <filesystem>
#include <format>
#include <iostream>
#include <sys/stat.h>

TEST_CASE("xcessively long URI does not crash server with filesystem exception")
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
	server.sandbox().writeFile("index.html", "ALIVE");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Generate 8 KB path to trigger ENAMETOOLONG
	std::string longPath = "/" + std::string(8192, 'a');
	HttpResponse res = client.get(longPath);

	// Must return an HTTP error (414, 404, 400), not terminate the server process
	TEST_ASSERT(res.statusCode == 414 || res.statusCode == 404 || res.statusCode == 400 || res.statusCode == 500);

	// Server must remain alive and serve subsequent requests
	HttpResponse aliveRes = client.get("/index.html");
	TEST_ASSERT_EQ(aliveRes.statusCode, 200);
	TEST_ASSERT_CONTAINS(aliveRes.body, "ALIVE");
}

TEST_CASE("Percent-encoded dot-dot traversal (%2e%2e) is rejected")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /public {
				methods GET;
				root ./public;
			}
		}
	)");
	server.sandbox().writeFile("secret.key", "CONFIDENTIAL_KEY_DATA");
	server.sandbox().writeFile("public/info.txt", "Public Info");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// %2e%2e = ..
	HttpResponse res1 = client.get("/public/%2e%2e/secret.key");
	TEST_ASSERT(res1.statusCode == 400 || res1.statusCode == 403 || res1.statusCode == 404);
	TEST_ASSERT(res1.body.find("CONFIDENTIAL_KEY_DATA") == std::string::npos);

	// %2e%2e%2f = ../
	HttpResponse res2 = client.get("/public/%2e%2e%2fsecret.key");
	TEST_ASSERT(res2.statusCode == 400 || res2.statusCode == 403 || res2.statusCode == 404);
	TEST_ASSERT(res2.body.find("CONFIDENTIAL_KEY_DATA") == std::string::npos);
}

TEST_CASE("DELETE on directory root does not recursively wipe contents")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /data {
				methods GET DELETE;
				root ./;
			}
		}
	)");

	server.sandbox().writeFile("data/important_file.txt", "CANNOT_BE_DELETED_BY_ROOT_DELETE");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Attempting DELETE on directory root
	HttpResponse delRes = client.deleteReq("/data/");
	TEST_ASSERT(delRes.statusCode == 400 || delRes.statusCode == 403 || delRes.statusCode == 405 ||
				delRes.statusCode == 409 || delRes.statusCode == 404);

	// File inside must still exist!
	HttpResponse getRes = client.get("/data/important_file.txt");
	TEST_ASSERT_EQ(getRes.statusCode, 200);
	TEST_ASSERT_CONTAINS(getRes.body, "CANNOT_BE_DELETED_BY_ROOT_DELETE");
}

TEST_CASE("Location prefix matching respects segment boundary (/admin vs /adminX)")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /admin {
				methods POST;
				root ./admin_dir;
			}
			location / {
				methods GET;
				root ./public_dir;
			}
		}
	)");

	server.sandbox().writeFile("public_dir/adminX.html", "PUBLIC_ADMINX_CONTENT");
	server.sandbox().writeFile("admin_dir/index.html", "ADMIN_SECRET");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// /adminX should match location / (GET allowed), not location /admin (POST only)
	HttpResponse res = client.get("/adminX.html");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "PUBLIC_ADMINX_CONTENT");
}

TEST_CASE("Invalid chunk size formats (spaces, signs, trailing junk) return 400")
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

	// 1. Negative sign chunk size "-3\r\n"
	std::string reqNegative = std::format("POST /chunk.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nTransfer-Encoding: "
										  "chunked\r\nConnection: close\r\n\r\n-3\r\nfoo\r\n0\r\n\r\n",
										  server.getPort());
	HttpResponse resNeg = client.sendRaw(reqNegative);
	TEST_ASSERT_EQ(resNeg.statusCode, 400);

	// 2. Leading space in chunk size " 5\r\n"
	std::string reqLeadingSpace = std::format("POST /chunk.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nTransfer-Encoding: "
											  "chunked\r\nConnection: close\r\n\r\n 5\r\nhello\r\n0\r\n\r\n",
											  server.getPort());
	HttpResponse resSpace = client.sendRaw(reqLeadingSpace);
	TEST_ASSERT_EQ(resSpace.statusCode, 400);

	// 3. Trailing non-hex characters "5xyz\r\n"
	std::string reqTrailingJunk = std::format("POST /chunk.txt HTTP/1.1\r\nHost: 127.0.0.1:{}\r\nTransfer-Encoding: "
											  "chunked\r\nConnection: close\r\n\r\n5xyz\r\nhello\r\n0\r\n\r\n",
											  server.getPort());
	HttpResponse resJunk = client.sendRaw(reqTrailingJunk);
	TEST_ASSERT_EQ(resJunk.statusCode, 400);
}

TEST_CASE("Duplicate Host headers in a single request return 400 (RFC 9112 §7.1)")
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

	std::string rawReq = std::format("GET / HTTP/1.1\r\n"
									 "Host: example.com\r\n"
									 "Host: duplicate.com\r\n"
									 "Connection: close\r\n\r\n");

	HttpResponse res = client.sendRaw(rawReq);
	TEST_ASSERT_EQ(res.statusCode, 400);
}

TEST_CASE("Path traversal via CGI path (/cgi-bin/../../) returns 403 Forbidden and blocks execution")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /cgi-bin {
				methods GET POST;
				root ./site;
				cgi .sh /bin/sh;
				cgi .py /usr/bin/python3;
			}
			location / {
				methods GET;
				root ./site;
			}
		}
	)");

	std::string maliciousScript = "#!/bin/sh\n"
								  "printf 'Content-Type: text/plain\\r\\n\\r\\n'\n"
								  "printf 'BADDD!\\n'\n";

	// 1. Place pwd.sh OUTSIDE ./site (in sandbox root)
	server.sandbox().writeFile("pwd.sh", maliciousScript);
	chmod((server.sandbox().getPath() / "pwd.sh").c_str(), 0755);

	// 2. Ensure ./site/cgi-bin exists so the path is structurally valid
	server.sandbox().writeFile("site/cgi-bin/.gitkeep", "");
	server.sandbox().writeFile("site/index.html", "SERVER_HEALTHY");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	// Simulates: curl --path-as-is localhost:{PORT}/cgi-bin/../../pwd.sh
	HttpResponse res = client.get("/cgi-bin/../../pwd.sh");

	TEST_ASSERT_EQ(res.statusCode, 403);
	TEST_ASSERT(res.body.find("BADDD!") == std::string::npos);

	// Ensure server is still alive and serving normal requests
	HttpResponse healthyRes = client.get("/index.html");
	TEST_ASSERT_EQ(healthyRes.statusCode, 200);
	TEST_ASSERT_CONTAINS(healthyRes.body, "SERVER_HEALTHY");
}
