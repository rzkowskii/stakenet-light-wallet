#include <QDebug>
#include <QSignalSpy>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <gtest/gtest.h>

#include <Networking/DockerApiClient.hpp>
#include <Networking/LocalSocketRequestHandlerImpl.hpp>

using namespace testing;
using namespace boost;

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

TEST(BeastTests, BeastHttpRequestSerialization)
{
    // Set up an HTTP GET request message
    http::request<http::string_body> req{ http::verb::get, "1.1", 11 };
    req.set(http::field::host, "www.example.com");
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    req.prepare_payload();

    std::string data;
    {
        std::ostringstream oss;
        oss << req;
        data = oss.str();
        qDebug() << data.c_str();
        oss.clear();
    }

    data = "HTTP/1.1 200 OK\r\nApi-Version: 1.40\r\nContent-Type: "
           "application/json\r\nDocker-Experimental: false\r\nOstype: linux\r\nServer: "
           "Docker/19.03.6 (linux)\r\nDate: Fri, 15 Jan 2021 12:21:36 GMT\r\nTransfer-Encoding: "
           "chunked\r\n\r\n854\r\n{\"ID\":\"VLSC:RFQB:DFTZ:X6HJ:5PZD:46TQ:EITN:VO2L:C23L:TS7O:K5JZ:"
           "IIZK\",\"Containers\":0,\"ContainersRunning\":0,\"ContainersPaused\":0,"
           "\"ContainersStopped\":0,\"Images\":7,\"Driver\":\"overlay2\",\"DriverStatus\":[["
           "\"Backing Filesystem\",\"extfs\"],[\"Supports d_type\",\"true\"],[\"Native Overlay "
           "Diff\",\"true\"]],\"SystemStatus\":null,\"Plugins\":{\"Volume\":[\"local\"],"
           "\"Network\":[\"bridge\",\"host\",\"ipvlan\",\"macvlan\",\"null\",\"overlay\"],"
           "\"Authorization\":null,\"Log\":[\"awslogs\",\"fluentd\",\"gcplogs\",\"gelf\","
           "\"journald\",\"json-file\",\"local\",\"logentries\",\"splunk\",\"syslog\"]},"
           "\"MemoryLimit\":true,\"SwapLimit\":false,\"KernelMemory\":true,\"KernelMemoryTCP\":"
           "true,\"CpuCfsPeriod\":true,\"CpuCfsQuota\":true,\"CPUShares\":true,\"CPUSet\":true,"
           "\"PidsLimit\":true,\"IPv4Forwarding\":true,\"BridgeNfIptables\":true,"
           "\"BridgeNfIp6tables\":true,\"Debug\":false,\"NFd\":21,\"OomKillDisable\":true,"
           "\"NGoroutines\":34,\"SystemTime\":\"2021-01-15T14:21:36.954785255+02:00\","
           "\"LoggingDriver\":\"json-file\",\"CgroupDriver\":\"cgroupfs\",\"NEventsListener\":0,"
           "\"KernelVersion\":\"5.4.0-60-generic\",\"OperatingSystem\":\"Ubuntu 18.04.5 "
           "LTS\",\"OSType\":\"linux\",\"Architecture\":\"x86_64\",\"IndexServerAddress\":\"https:/"
           "/index.docker.io/v1/"
           "\",\"RegistryConfig\":{\"AllowNondistributableArtifactsCIDRs\":[],"
           "\"AllowNondistributableArtifactsHostnames\":[],\"InsecureRegistryCIDRs\":[\"127.0.0.0/"
           "8\"],\"IndexConfigs\":{\"docker.io\":{\"Name\":\"docker.io\",\"Mirrors\":[],\"Secure\":"
           "true,\"Official\":true}},\"Mirrors\":[]},\"NCPU\":4,\"MemTotal\":16447131648,"
           "\"GenericResources\":null,\"DockerRootDir\":\"/home/durkmurder/"
           "docker\",\"HttpProxy\":\"\",\"HttpsProxy\":\"\",\"NoProxy\":\"\",\"Name\":\"durkmurder-"
           "ThinkPad-T430\",\"Labels\":[],\"ExperimentalBuild\":false,\"ServerVersion\":\"19.03."
           "6\",\"ClusterStore\":\"\",\"ClusterAdvertise\":\"\",\"Runtimes\":{\"runc\":{\"path\":"
           "\"runc\"}},\"DefaultRuntime\":\"runc\",\"Swarm\":{\"NodeID\":\"\",\"NodeAddr\":\"\","
           "\"LocalNodeState\":\"inactive\",\"ControlAvailable\":false,\"Error\":\"\","
           "\"RemoteManagers\":null},\"LiveRestoreEnabled\":false,\"Isolation\":\"\","
           "\"InitBinary\":\"docker-init\",\"ContainerdCommit\":{\"ID\":\"\",\"Expected\":\"\"},"
           "\"RuncCommit\":{\"ID\":\"\",\"Expected\":\"\"},\"InitCommit\":{\"ID\":\"\","
           "\"Expected\":\"\"},\"SecurityOptions\":[\"name=apparmor\",\"name=seccomp,profile="
           "default\"],\"Warnings\":[\"WARNING: No swap limit support\"]}\n\r\n0\r\n\r\n";

    // Declare a container to hold the response
    http::response_parser<http::dynamic_body> parser;
    boost::system::error_code ec;
    size_t bytesConsumed = 0;
    while(bytesConsumed < data.size()) {
        auto buffer = asio::buffer(&data[bytesConsumed], data.size() - bytesConsumed);
        bytesConsumed += parser.put(buffer, ec);
        ASSERT_FALSE(ec);
    }
    http::response<http::dynamic_body> res = parser.get();
    ASSERT_EQ(res.result_int(), 200);
    auto bodyAsString = boost::beast::buffers_to_string(res.body().data());
    ASSERT_GT(bodyAsString.size(), 0);
}

TEST(DockerApiClientTests, DockerGetInfo)
{
    DockerApiClient apiClient(std::make_unique<LocalSocketRequestHandlerImpl>());
    QVariantMap expectedData;
    apiClient.info()
        .then([&expectedData](QVariantMap data) { expectedData = data; })
        .timeout(5000)
        .wait();

    ASSERT_GT(expectedData.size(), 0);
}
