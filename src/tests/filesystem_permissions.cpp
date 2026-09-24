#include "TestContext.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"
#include <sys/stat.h>

TEST_CASE("Static file without read permissions returns 403 Forbidden")
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

	server.sandbox().writeFile("unreadable.txt", "TOP_SECRET_CANNOT_READ");
	chmod((server.sandbox().getPath() / "unreadable.txt").c_str(), 0000);
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/unreadable.txt");
	TEST_ASSERT_EQ(res.statusCode, 403);
}

TEST_CASE("Location with custom index directive serves configured index file")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /docs {
				methods GET;
				root ./;
				index welcome.html;
			}
		}
	)");

	server.sandbox().writeFile("docs/welcome.html", "WELCOME_DOCS_INDEX");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse res = client.get("/docs/");
	TEST_ASSERT_EQ(res.statusCode, 200);
	TEST_ASSERT_CONTAINS(res.body, "WELCOME_DOCS_INDEX");
}

TEST_CASE("Static files return appropriate Content-Type MIME headers")
{
	ServerInstance server = ctx.spawnServer(R"(
		server {
			listen 127.0.0.1:{PORT};
			location /assets {
				methods GET;
				root ./;
			}
		}
	)");

	server.sandbox().writeFile("assets/style.css", "body { color: red; }");
	server.sandbox().writeFile("assets/script.js", "console.log('test');");
	server.sandbox().writeFile("assets/note.txt", "plain note");
	server.sandbox().writeFile("assets/image.png", "\x89PNG\r\n\x1a\n");
	server.start();

	HttpClient client = server.client();
	TEST_ASSERT(client.waitForServer());

	HttpResponse resCss = client.get("/assets/style.css");
	TEST_ASSERT_EQ(resCss.statusCode, 200);
	TEST_ASSERT_CONTAINS(resCss.getHeaders("Content-Type"), "text/css");

	HttpResponse resJs = client.get("/assets/script.js");
	TEST_ASSERT_EQ(resJs.statusCode, 200);
	TEST_ASSERT_CONTAINS(resJs.getHeaders("Content-Type"), "application/javascript");

	HttpResponse resTxt = client.get("/assets/note.txt");
	TEST_ASSERT_EQ(resTxt.statusCode, 200);
	TEST_ASSERT_CONTAINS(resTxt.getHeaders("Content-Type"), "text/plain");

	HttpResponse resPng = client.get("/assets/image.png");
	TEST_ASSERT_EQ(resPng.statusCode, 200);
	TEST_ASSERT_CONTAINS(resPng.getHeaders("Content-Type"), "image/png");
}
