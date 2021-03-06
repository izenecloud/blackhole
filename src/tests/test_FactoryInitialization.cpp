#include <blackhole/formatter/string.hpp>
#include <blackhole/formatter/json.hpp>
#include <blackhole/frontend.hpp>
#include <blackhole/frontend/files.hpp>
#include <blackhole/frontend/syslog.hpp>
#include <blackhole/repository.hpp>
#include <blackhole/sink/files.hpp>
#include <blackhole/sink/socket.hpp>
#include <blackhole/sink/stream.hpp>
#include <blackhole/sink/syslog.hpp>

#include "global.hpp"

using namespace blackhole;

namespace blackhole { namespace sink {

template<>
struct priority_traits<testing::level> {
    static inline
    priority_t map(testing::level lvl) {
        switch (lvl) {
        case testing::level::debug:
            return priority_t::debug;
        case testing::level::info:
            return priority_t::info;
        case testing::level::warn:
            return priority_t::warning;
        case testing::level::error:
            return priority_t::err;
        }

        return priority_t::debug;
    }
};

} } // namespace blackhole::sink

TEST(Factory, StreamStringsFrontend) {
    factory_t factory;
    factory.add<sink::stream_t, formatter::string_t>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("stream");
    sink["output"] = "stdout";

    auto result = factory.create(formatter, sink);
    auto casted = dynamic_cast<
        frontend_t<formatter::string_t, sink::stream_t>*
    >(result.get());
    EXPECT_TRUE(casted != nullptr);
}

TEST(Factory, FileStringsFrontend) {
    factory_t factory;
    factory.add<sink::files_t<>, formatter::string_t>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;

    auto result = factory.create(formatter, sink);
    auto casted = dynamic_cast<
        frontend_t<formatter::string_t, sink::files_t<>>*
    >(result.get());
    EXPECT_TRUE(casted != nullptr);
}

TEST(Repository, RotationFileStringsFrontendWithMoveWatcher) {
    typedef frontend_t<
        formatter::string_t,
        sink::files_t<
            sink::files::boost_backend_t,
            sink::rotator_t<
                sink::files::boost_backend_t,
                sink::rotation::watcher::move_t
            >
        >
    > frontend_type;

    factory_t factory;
    factory.add<frontend_type::sink_type, frontend_type::formatter_type>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;
    sink["rotation"]["move"] = true;

    auto casted = dynamic_cast<frontend_type*>(
        factory.create(formatter, sink).get()
    );
    EXPECT_TRUE(casted != nullptr);
}

TEST(Repository, SupportsMultipleFileRotationPolicies) {
    typedef frontend_t<
        formatter::string_t,
        sink::files_t<
            sink::files::boost_backend_t,
            sink::rotator_t<
                sink::files::boost_backend_t,
                sink::rotation::watcher::move_t
            >
        >
    > frontend1_type;

    typedef frontend_t<
        formatter::string_t,
        sink::files_t<
            sink::files::boost_backend_t,
            sink::rotator_t<
                sink::files::boost_backend_t,
                sink::rotation::watcher::size_t
            >
        >
    > frontend2_type;

    factory_t factory;
    factory.add<frontend1_type::sink_type, frontend1_type::formatter_type>();
    factory.add<frontend2_type::sink_type, frontend2_type::formatter_type>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;
    sink["rotation"]["move"] = true;

    {
        auto casted = dynamic_cast<frontend1_type*>(
            factory.create(formatter, sink).get()
        );
        EXPECT_TRUE(casted != nullptr);
    }

    sink = sink_config_t("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;
    sink["rotation"]["pattern"] = "%(filename)s.log.%N";
    sink["rotation"]["backups"] = 5;
    sink["rotation"]["size"] = 10 * 1024 * 1024;

    {
        auto casted = dynamic_cast<frontend2_type*>(
            factory.create(formatter, sink).get()
        );
        EXPECT_TRUE(casted != nullptr);
    }
}

TEST(Repository, RotationFileStringsFrontendWithSizeWatcher) {
    typedef frontend_t<
        formatter::string_t,
        sink::files_t<
            sink::files::boost_backend_t,
            sink::rotator_t<
                sink::files::boost_backend_t,
                sink::rotation::watcher::size_t
            >
        >
    > frontend_type;

    factory_t factory;
    factory.add<frontend_type::sink_type, frontend_type::formatter_type>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;
    sink["rotation"]["pattern"] = "%(filename)s.log.%N";
    sink["rotation"]["backups"] = 5;
    sink["rotation"]["size"] = 10 * 1024 * 1024;

    auto casted = dynamic_cast<frontend_type*>(
        factory.create(formatter, sink).get()
    );
    EXPECT_TRUE(casted != nullptr);
}

TEST(Repository, RotationFileStringsFrontendWithDatetimeWatcher) {
    typedef frontend_t<
        formatter::string_t,
        sink::files_t<
            sink::files::boost_backend_t,
            sink::rotator_t<
                sink::files::boost_backend_t,
                sink::rotation::watcher::datetime_t<>
            >
        >
    > frontend_type;
    factory_t factory;
    factory.add<frontend_type::sink_type, frontend_type::formatter_type>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;
    sink["rotation"]["pattern"] = "%(filename)s.log.%N";
    sink["rotation"]["backups"] = 5;
    sink["rotation"]["period"] = "d";

    auto casted = dynamic_cast<frontend_type*>(
        factory.create(formatter, sink).get()
    );
    EXPECT_TRUE(casted != nullptr);
}

TEST(Repository, ThrowsExceptionIfRotationWatcherNotSpecified) {
    factory_t factory;
    factory.add<
        sink::files_t<
            sink::files::boost_backend_t,
            sink::rotator_t<
                sink::files::boost_backend_t,
                sink::rotation::watcher::datetime_t<>
            >
        >,
        formatter::string_t
    >();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;
    sink["rotation"]["pattern"] = "%(filename)s.log.%N";
    sink["rotation"]["backups"] = 5;

    EXPECT_THROW(factory.create(formatter, sink), blackhole::error_t);
}

TEST(Factory, SyslogStringsFrontend) {
    factory_t factory;
    factory.add<sink::syslog_t<level>, formatter::string_t>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("syslog");
    sink["identity"] = "AppIdentity";

    EXPECT_TRUE(bool(factory.create(formatter, sink)));
}

TEST(Factory, UdpSocketStringsFrontend) {
    factory_t factory;
    factory.add<sink::socket_t<boost::asio::ip::udp>, formatter::string_t>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("udp");
    sink["host"] = "localhost";
    sink["port"] = 50030;

    EXPECT_TRUE(bool(factory.create(formatter, sink)));
}

TEST(Factory, TcpSocketStringsFrontend) {
    factory_t factory;
    factory.add<sink::socket_t<boost::asio::ip::tcp>, formatter::string_t>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    // Sink will try to connect to the specified port. So if it isn't available,
    // an exception will be thrown, it's ok.
    sink_config_t sink("tcp");
    sink["host"] = "localhost";
    sink["port"] = 22;

    EXPECT_TRUE(bool(factory.create(formatter, sink)));
}

log_config_t create_valid_config() {
    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("stream");
    sink["output"] = "stdout";

    frontend_config_t frontend = { formatter, sink };
    return log_config_t{ "root", { frontend } };
}

TEST(Repository, InitializationFromSettings) {
    repository_t repository;
    repository.add_config(create_valid_config());

    const bool condition = std::is_same<
        verbose_logger_t<testing::level>,
        decltype(repository.create<verbose_logger_t<testing::level>>("root"))
    >::value;
    static_assert(condition, "repository should return `verbose_logger_t` object");
}

TEST(Repository, ThrowsExceptionIfLoggerWithSpecifiedNameNotFound) {
    repository_t repository;
    repository.add_config(create_valid_config());
    EXPECT_THROW(repository.create<verbose_logger_t<testing::level>>("log"),
                 std::out_of_range);
}

TEST(Repository, CreatesDuplicateOfRootLoggerByDefault) {
    repository_t repository;
    log_config_t config = create_valid_config();
    repository.add_config(config);
    repository.create<verbose_logger_t<testing::level>>("root");
}

TEST(Factory, ThrowsExceptionWhenRequestNotRegisteredSink) {
    factory_t factory;

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;

    EXPECT_THROW(factory.create(formatter, sink), blackhole::error_t);
}

TEST(Factory, ThrowsExceptionWhenRequestNotRegisteredFormatter) {
    factory_t factory;
    factory.add<sink::socket_t<boost::asio::ip::udp>, boost::mpl::list<>>();

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;

    EXPECT_THROW(factory.create(formatter, sink), blackhole::error_t);
}

TEST(Repository, StreamSinkWithStringFormatterIsAvailableByDefault) {
    repository_t repository;
    bool registered = repository.registered<
        sink::stream_t,
        formatter::string_t
    >();
    EXPECT_TRUE(registered);
}

TEST(Repository, PairConfiguring) {
    bool registered = false;
    repository_t repository;

    registered = repository.registered<
        sink::syslog_t<level>,
        formatter::string_t
    >();
    EXPECT_FALSE(registered);

    repository.registrate<sink::syslog_t<level>, formatter::string_t>();

    registered = repository.registered<
        sink::syslog_t<level>,
        formatter::string_t
    >();
    EXPECT_TRUE(registered);
}

TEST(Repository, ConfiguringWithMultipleFormatters) {
    typedef boost::mpl::list<
        formatter::string_t,
        formatter::json_t
    > formatters_t;

    bool registered = false;
    repository_t repository;

    registered = repository.registered<sink::files_t<>, formatter::json_t>();
    EXPECT_FALSE(registered);

    repository.registrate<sink::files_t<>, formatters_t>();

    registered = repository.registered<sink::files_t<>, formatter::json_t>();
    EXPECT_TRUE(registered);
}

TEST(Repository, CombinationConfiguring) {
    typedef boost::mpl::vector<
        sink::files_t<>,
        sink::syslog_t<level>
    > sinks_t;

    typedef boost::mpl::list<
        formatter::string_t,
        formatter::json_t
    > formatters_t;

    bool registered = false;
    repository_t repository;

    registered = repository.registered<sink::files_t<>, formatter::json_t>();
    ASSERT_FALSE(registered);
    registered = repository.registered<sink::syslog_t<level>, formatter::string_t>();
    ASSERT_FALSE(registered);
    registered = repository.registered<sink::syslog_t<level>, formatter::json_t>();
    ASSERT_FALSE(registered);

    repository.registrate<sinks_t, formatters_t>();

    registered = repository.registered<sink::files_t<>, formatter::json_t>();
    EXPECT_TRUE(registered);
    registered = repository.registered<sink::syslog_t<level>, formatter::string_t>();
    EXPECT_TRUE(registered);
    registered = repository.registered<sink::syslog_t<level>, formatter::json_t>();
    EXPECT_TRUE(registered);
}

TEST(Repository, ThrowsExceptionWhenSinkIsRegisteredTypeIsDifferent) {
    repository_t repository;

    formatter_config_t formatter("string");
    formatter["pattern"] = "[%(timestamp)s]: %(message)s";

    sink_config_t sink("files");
    sink["path"] = "/dev/stdout";
    sink["autoflush"] = true;
    sink["rotation"]["size"] = 1000;

    frontend_config_t frontend = { formatter, sink };
    log_config_t config = { "root", { frontend } };

    repository.registrate<
        sink::files_t<
            sink::files::boost_backend_t,
            sink::rotator_t<
                sink::files::boost_backend_t,
                sink::rotation::watcher::size_t
            >
        >,
        formatter::string_t
    >();
    repository.add_config(config);

    EXPECT_THROW(repository.create<verbose_logger_t<level>>("root"),
                 blackhole::error_t);
}
