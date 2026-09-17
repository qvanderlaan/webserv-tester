#include "HttpClient.hpp"
#include "ITestCase.hpp"
#include "ServerInstance.hpp"
#include "TestRegistry.hpp"
#include "assertions.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class Return200ForRootRequest : public ITestCase
{
	public:
		std::string getName(void) const override
		{
			return ("Return 200 for root request");
		}

		fs::path getStorageDir(void) const
		{
			return (fs::canonical("src/tests/return_200_for_root_request/storage"));
		}

		void run(const fs::path& webservBin) override
		{
			ServerInstance server(webservBin, this->getStorageDir(), "main.conf");
			server.start();

			HttpClient client("127.0.0.1", 8080);
			TEST_ASSERT(client.waitForServer());

			HttpResponse res = client.get("/", {});

			TEST_ASSERT_EQ(res.statusCode, 200);
			TEST_ASSERT_CONTAINS(res.body, "<h1>Hello World!</h1>");
		}
};

REGISTER_TEST(Return200ForRootRequest);
