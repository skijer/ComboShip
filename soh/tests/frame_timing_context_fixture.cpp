// Lightweight Context owner for the ROM-free local test. Only subsystem setup
// is substituted: the production probe, async logger, file sink and JSON are real.
// The Windows build runs the same test against the real libultraship.dll instead.
#include <ship/Context.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace Ship {
std::unique_ptr<Context> Context::mContext;

Context::Context(std::string name, std::string shortName, std::string configFilePath)
    : mConfigFilePath(std::move(configFilePath)), mName(std::move(name)), mShortName(std::move(shortName)) {
}

Context::~Context() {
    mLogger->flush();
    spdlog::shutdown();
    mLogger.reset();
    mLogThreadPool.reset();
}

Context* Context::CreateUninitializedInstance(const std::string& name, const std::string& shortName,
                                              const std::string& configFilePath) {
    mContext = std::make_unique<Context>(name, shortName, configFilePath);
    return mContext.get();
}

Context* Context::GetRawInstance() {
    return mContext.get();
}

void Context::DestroyInstance() {
    mContext.reset();
}

std::shared_ptr<spdlog::logger> Context::GetLogger() const {
    return mLogger;
}

std::string Context::GetPathRelativeToAppDirectory(const std::string& path, const std::string&) {
    return "./" + path;
}

bool Context::InitLogging(spdlog::level::level_enum, spdlog::level::level_enum releaseLevel) {
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/" + mName + ".log", 1024 * 1024, 1);
    mLogThreadPool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
    mLogger = std::make_shared<spdlog::async_logger>(mName, sink, mLogThreadPool, spdlog::async_overflow_policy::block);
    mLogger->set_level(releaseLevel);
    mLogger->flush_on(spdlog::level::info);
    spdlog::set_default_logger(mLogger);
    return true;
}
} // namespace Ship
