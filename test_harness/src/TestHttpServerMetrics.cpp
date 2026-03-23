/*
    Oscil Test Harness - HTTP Server: Performance Metrics Handlers
*/

#include "TestHttpServer.h"

namespace oscil::test
{

void TestHttpServer::setupMetricsRoutes()
{
    server_->Post("/metrics/start",
                  [this](const httplib::Request& req, httplib::Response& res) { handleMetricsStart(req, res); });
    server_->Post("/metrics/stop",
                  [this](const httplib::Request& req, httplib::Response& res) { handleMetricsStop(req, res); });
    server_->Get("/metrics/current",
                 [this](const httplib::Request& req, httplib::Response& res) { handleMetricsCurrent(req, res); });
    server_->Get("/metrics/stats",
                 [this](const httplib::Request& req, httplib::Response& res) { handleMetricsStats(req, res); });
    server_->Post("/metrics/reset",
                  [this](const httplib::Request& req, httplib::Response& res) { handleMetricsReset(req, res); });
    server_->Post("/metrics/recordFrame",
                  [this](const httplib::Request& req, httplib::Response& res) { handleMetricsRecordFrame(req, res); });
}

void TestHttpServer::handleMetricsStart(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        auto body = json::parse(req.body);
        int intervalMs = body.value("intervalMs", 100);

        metrics_.startCollection(intervalMs);

        json data;
        data["collecting"] = metrics_.isCollecting();
        data["intervalMs"] = intervalMs;
        res.set_content(successResponse(data).dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        res.set_content(errorResponse(e.what()).dump(), "application/json");
    }
}

void TestHttpServer::handleMetricsStop(const httplib::Request&, httplib::Response& res)
{
    metrics_.stopCollection();

    json data;
    data["collecting"] = metrics_.isCollecting();
    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleMetricsCurrent(const httplib::Request&, httplib::Response& res)
{
    auto snapshot = metrics_.getCurrentSnapshot();

    json data;
    data["fps"] = snapshot.fps;
    data["cpuPercent"] = snapshot.cpuPercent;
    data["memoryBytes"] = snapshot.memoryBytes;
    data["memoryMB"] = snapshot.memoryMB;
    data["oscillatorCount"] = snapshot.oscillatorCount;
    data["sourceCount"] = snapshot.sourceCount;
    data["timestamp"] = snapshot.timestamp;
    data["collecting"] = metrics_.isCollecting();

    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleMetricsStats(const httplib::Request& req, httplib::Response& res)
{
    int periodMs = 0;
    auto periodParam = req.get_param_value("periodMs");
    if (!periodParam.empty())
        periodMs = std::stoi(periodParam);

    PerformanceStats stats;
    if (periodMs > 0)
        stats = metrics_.getStatsForPeriod(periodMs);
    else
        stats = metrics_.getStats();

    json data;
    data["avgFps"] = stats.avgFps;
    data["minFps"] = stats.minFps;
    data["maxFps"] = stats.maxFps;
    data["avgCpu"] = stats.avgCpu;
    data["maxCpu"] = stats.maxCpu;
    data["avgMemoryMB"] = stats.avgMemoryMB;
    data["maxMemoryMB"] = stats.maxMemoryMB;
    data["sampleCount"] = stats.sampleCount;
    data["durationMs"] = stats.durationMs;

    res.set_content(successResponse(data).dump(), "application/json");
}

void TestHttpServer::handleMetricsReset(const httplib::Request&, httplib::Response& res)
{
    metrics_.reset();
    res.set_content(successResponse().dump(), "application/json");
}

void TestHttpServer::handleMetricsRecordFrame(const httplib::Request&, httplib::Response& res)
{
    metrics_.recordFrame();
    res.set_content(successResponse().dump(), "application/json");
}

} // namespace oscil::test
