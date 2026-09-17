#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"

TEST_CASE("Client body size exceeding limit returns 413")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			client_max_body_size 10;
			location / {
				methods POST;
				root ./;
			}
		}
	)");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	std::string largeBody = "This string has way more than 10 bytes!";
	HttpResponse res = client.post("/upload.txt", largeBody);
	TEST_ASSERT_EQ(res.statusCode, 413);
}

TEST_CASE("Custom error page is served on 404")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			error_page 404 /custom_404.html;
			location / {
				methods GET;
				root ./;
			}
		}
	)");

	server.sandbox().writeFile("custom_404.html", "<h1>Custom 404 Not Found Page</h1>");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/this_file_does_not_exist.html");
	TEST_ASSERT_EQ(res.statusCode, 404);
	TEST_ASSERT_CONTAINS(res.body, "Custom 404 Not Found Page");
}

TEST_CASE("Autoindex on lists directory contents when no index exists")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /files {
				methods GET;
				root ./;
				autoindex on;
			}
		}
	)");

	server.sandbox().writeFile("files/document_a.txt", "Alpha");
	server.sandbox().writeFile("files/document_b.txt", "Beta");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/files/");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "document_a.txt");
	TEST_ASSERT_CONTAINS(res.body, "document_b.txt");
}

TEST_CASE("Autoindex off on directory without index returns 403 or 404")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /secret_dir {
				methods GET;
				root ./;
				autoindex off;
			}
		}
	)");

	server.sandbox().writeFile("secret_dir/data.txt", "Secret");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/secret_dir/");
	TEST_ASSERT(res.statusCode == 403 || res.statusCode == 404);
}

TEST_CASE("HTTP Redirection returns 301/302 and Location header")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /old-path {
				methods GET;
				redirect 301 /new-path;
			}
			location /new-path {
				methods GET;
				root ./;
				index target.html;
			}
		}
	)");

	server.sandbox().writeFile("target.html", "Reached Target");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/old-path");
	TEST_ASSERT(res.statusCode == 301 || res.statusCode == 302 || res.statusCode == 307 || res.statusCode == 308);
	TEST_ASSERT_CONTAINS(res.getHeaders("Location"), "/new-path");
}
