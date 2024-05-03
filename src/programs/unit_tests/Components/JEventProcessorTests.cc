
#include "catch.hpp"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>

struct MyEventProcessor : public JEventProcessor {
    int init_count = 0;
    int process_count = 0;
    int finish_count = 0;
    int* destroy_count = nullptr;

    MyEventProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        SetTypeName(NAME_OF_THIS);
    }
    ~MyEventProcessor() {
        (*destroy_count)++;
    }
    void Init() override {
        LOG_INFO(GetLogger()) << "Init() called" << LOG_END;
        init_count++;
    }
    void Process(const JEvent&) override {
        process_count++;
        LOG_INFO(GetLogger()) << "Process() called" << LOG_END;
    }
    void Finish() override {
        LOG_INFO(GetLogger()) << "Finish() called" << LOG_END;
        finish_count++;
    }
};

TEST_CASE("JEventProcessor_ExpertMode_ProcessCount") {

    LOG << "Running test: JEventProcessor_ExpertMode_ProcessCount" << LOG_END;

    auto sut = new MyEventProcessor;
    int destroy_count = 0;
    sut->destroy_count = &destroy_count;

    {
        JApplication app;
        app.SetParameterValue("log:global", "off");
        app.SetParameterValue("log:info", "MyEventProcessor");
        app.SetParameterValue("jana:nevents", 5);

        app.Add(new JEventSource);
        app.Add(sut);
        app.Run();

        REQUIRE(sut->init_count == 1);
        REQUIRE(sut->process_count == 5);
        REQUIRE(sut->GetEventCount() == 5);
        REQUIRE(sut->finish_count == 1);
        REQUIRE(destroy_count == 0);

        // App goes out of scope; sut should be destroyed
    }
    REQUIRE(destroy_count == 1);
}

struct MyExceptingProcessor : public JEventProcessor {
    void Process(const std::shared_ptr<const JEvent>&) override {
        throw std::runtime_error("Mystery!");
    }
};

TEST_CASE("JEventProcessor_Exception") {
    JApplication app;
    app.Add(new JEventSource);
    app.Add(new MyExceptingProcessor);
    bool found_throw = false;
    try {
        app.Run();
    }
    catch(JException& ex) {
        REQUIRE(ex.function_name == "JEventProcessor::Process");
        REQUIRE(ex.message == "Mystery!");
        REQUIRE(ex.exception_type == "std::runtime_error");
        found_throw = true;
    }
    REQUIRE(found_throw == true);

}
