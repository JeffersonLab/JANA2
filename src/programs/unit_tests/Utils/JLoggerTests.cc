
#include <catch.hpp>
#include <iostream>
#include <JANA/JLogger.h>


TEST_CASE("JLogMessage_DestructorOrdering") {

    {
        JLogMessage m("JANA: ");
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
        JLogMessage msg(logger, JLogger::Level::INFO);
        msg << "2. This will print at level INFO and include" << std::endl;
        msg << "   multiple lines and the group name 'jana'" << std::endl;
        msg << "   It will destruct immediately because it is inside an if-block";
    }

    std::cout << std::endl;

    if (logger.level >= JLogger::Level::WARN) JLogMessage("jaanaa >> ") << "3. This destructs immediately even without enclosing braces";

    std::cout << std::endl;

    JLogMessage("Silly: ") << "4. This message will destruct immediately because it is an rvalue";

    std::cout << std::endl;

    std::cout << "6. This should be the last thing printed, assuming our destructors behave" << std::endl;

}

TEST_CASE("JLogMessage_Newlines") {

    JLogMessage("jaanaa> ") << "1. This message has a trailing newline in the code but shouldn't in the output" << std::endl;
    JLogMessage("jaanaa> ") << "2. This message has no trailing newline in either the code or the output";

    std::cout << "--------" << std::endl;
    std::cout << "The following line should just be log metadata with no log message" << std::endl;
    JLogMessage("jaanaa> ") << std::endl;
    std::cout << "--------" << std::endl;

    JLogMessage("jaanaa> ") << "There should be a line of just log metadata below this one" << std::endl << std::endl;
    JLogMessage("jaanaa> ") << "This should be the last line prefixed with 'jaanaa'.";

    JLogger logger {JLogger::Level::DEBUG, &std::cout, "jana"};

    LOG_INFO(logger) << "This message has a trailing newline in the code but shouldn't in the output" << std::endl;
    LOG_INFO(logger) << "This message has a trailing newline in the code but shouldn't in the output" << LOG_END;

    LOG_INFO(logger) << "This message has a trailing newline containing log metadata" << std::endl << std::endl;
    LOG_INFO(logger) << "This message has a trailing newline containing log metadata" << LOG_END << LOG_END;
    LOG_INFO(logger) << "This message has a trailing newline containing log metadata " << std::endl << LOG_END;
}

