
#include <catch.hpp>
#include <iostream>
#include <JANA/JLogger.h>

class JLogMsg : public std::stringstream {
public:
    struct Flush {};
    std::string m_prefix;
    JLogger* m_logger = nullptr;

public:
    JLogMsg(const std::string& prefix="") : m_prefix(prefix){
    }

    JLogMsg(const JLogger& logger, JLogger::Level level) {
        std::ostringstream builder;
        if (logger.show_timestamp) {
            auto now = std::chrono::system_clock::now();
            std::time_t current_time = std::chrono::system_clock::to_time_t(now);
            tm tm_buf;
            localtime_r(&current_time, &tm_buf);

            // Extract milliseconds by calculating the duration since the last whole second
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            builder << std::put_time(&tm_buf, "%H:%M:%S.");
            builder << std::setfill('0') << std::setw(3) << milliseconds.count() << std::setfill(' ') << " ";
        }
        if (logger.show_level) {
            switch (level) {
                case JLogger::Level::TRACE: builder << "[trace] "; break;
                case JLogger::Level::DEBUG: builder << "[debug] "; break;
                case JLogger::Level::INFO:  builder << " [info] "; break;
                case JLogger::Level::WARN:  builder << " [warn] "; break;
                case JLogger::Level::ERROR: builder << "[error] "; break;
                case JLogger::Level::FATAL: builder << "[fatal] "; break;
                default: builder << "[?????] ";
            }
        }
        if (logger.show_threadstamp) {
            builder << std::this_thread::get_id() << " ";
        }
        if (logger.show_group && !logger.group.empty()) {
            builder << logger.group << " > ";
        }
        m_prefix = builder.str();
    }

    void do_flush() {
        std::string line;
        std::ostringstream oss;
        while (std::getline(*this, line)) {
            oss << m_prefix << line << std::endl;
        }
        std::cout << oss.str();
        std::cout.flush();
        this->str("");
        this->clear();
    }

    virtual ~JLogMsg() {
        do_flush();
    }
};

inline JLogMsg& operator<<(JLogMsg& msg, const JLogMsg::Flush&) {
    // This lets us force a flush e.g. for use with LOG_END.
    msg.do_flush();
    return msg;
}


TEST_CASE("JLogMessage_DestructorOrdering") {

    {
        JLogMsg m("JANA: ");
        // This will destruct immediately, but only because it's in its own scope
        m << "1. This is a test";
        REQUIRE(m.str() == "1. This is a test");

        m << std::endl << "   which continues on the second line";
        REQUIRE(m.str() == "1. This is a test\n   which continues on the second line");
    }

    std::cout << std::endl;

    JLogger logger {JLogger::Level::WARN, &std::cout, "jana"};
    logger.show_group = true;
    if (logger.level >= JLogger::Level::WARN) {
        JLogMsg msg(logger, JLogger::Level::INFO);
        msg << "2. This will print at level INFO and include" << std::endl;
        msg << "   multiple lines and the group name 'jana'" << std::endl;
        msg << "   It will destruct immediately because it is inside an if-block";
    }

    std::cout << std::endl;

    if (logger.level >= JLogger::Level::WARN) JLogMsg("jaanaa >> ") << "3. This destructs immediately even without enclosing braces";

    std::cout << std::endl;

    JLogMsg("Silly: ") << "4. This message will destruct immediately because it is an rvalue";

    std::cout << std::endl;

    JLogMsg m("JANA: ");
    // This will destruct immediately because we include a LOG_END-style flush
    m << "5. This is a JLogMsg that is NOT in its own scope, but we will manually flush it at the right time.";
    m << JLogMsg::Flush();
    m << "   This is a line that got emitted as part of the same JLogMsg, but after the flush." << std::endl;
    m << "   It needs one final flush, else it will destruct too late and these lines will be printed out of order.";
    m << JLogMsg::Flush();

    std::cout << std::endl;

    std::cout << "6. This should be the last thing printed, assuming our destructors behave" << std::endl;

}

