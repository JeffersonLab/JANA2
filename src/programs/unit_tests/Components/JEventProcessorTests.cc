
#include "catch.hpp"

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>

namespace jana::jeventprocessortests {


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
        REQUIRE(GetApplication() != nullptr);
        LOG_INFO(GetLogger()) << "Init() called" << LOG_END;
        init_count++;
    }
    void ProcessSequential(const JEvent&) override {
        REQUIRE(GetApplication() != nullptr);
        process_count++;
        LOG_INFO(GetLogger()) << "ProcessSequential() called" << LOG_END;
    }
    void Finish() override {
        REQUIRE(GetApplication() != nullptr);
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
        app.SetParameterValue("jana:loglevel", "off");
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


struct SourceWithRunNumberChange : public JEventSource {
    SourceWithRunNumberChange() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    Result Emit(JEvent& event) {
        if (GetEmittedEventCount() < 2) {
            event.SetRunNumber(48);
        }
        else {
            event.SetRunNumber(49);
        }
        return Result::Success;
    }
};


struct ProcWithExceptionsAndLogging : public JEventProcessor {
    Parameter<bool> except_on_init {this, "except_on_init", false, "Except on init"};
    Parameter<bool> except_on_beginrun {this, "except_on_beginrun", false, "Except on beginrun"};
    Parameter<bool> except_on_process {this, "except_on_process", false, "Except on process"};
    Parameter<bool> except_on_endrun {this, "except_on_endrun", false, "Except on endrun"};
    Parameter<bool> except_on_finish {this, "except_on_finish", false, "Except on finish"};

    std::vector<std::string> log;

    void Init() override {
        LOG_INFO(GetLogger()) << "ProcWithExceptionsAndLogging::Init" << LOG_END;
        log.push_back("init");
        if (*except_on_init) throw std::runtime_error("Mystery");
    }
    void BeginRun(const std::shared_ptr<const JEvent>&) override {
        LOG_INFO(GetLogger()) << "ProcWithExceptionsAndLogging::BeginRun" << LOG_END;
        log.push_back("beginrun");
        if (*except_on_beginrun) throw std::runtime_error("Mystery");
    }
    void Process(const std::shared_ptr<const JEvent>&) override {
        LOG_INFO(GetLogger()) << "ProcWithExceptionsAndLogging::Process" << LOG_END;
        log.push_back("process");
        if (*except_on_process) throw std::runtime_error("Mystery");
    }
    void EndRun() override {
        LOG_INFO(GetLogger()) << "ProcWithExceptionsAndLogging::EndRun" << LOG_END;
        log.push_back("endrun");
        if (*except_on_endrun) throw std::runtime_error("Mystery");
    }
    void Finish() override {
        LOG_INFO(GetLogger()) << "ProcWithExceptionsAndLogging::Finish" << LOG_END;
        log.push_back("finish");
        if (*except_on_finish) throw std::runtime_error("Mystery");
    }
};

TEST_CASE("JEventProcessor_CallbackSequence") {
    JApplication app;
    auto sut = new ProcWithExceptionsAndLogging;
    app.Add(sut);
    // JApplication takes ownership of sut, so our pointer will become invalid when JApplication is destroyed

    SECTION("NoRunNumber") {
        app.Add(new JEventSource);
        app.SetParameterValue("jana:nevents", 2);
        app.Run();
        REQUIRE(sut->log.size() == 6);
        REQUIRE(sut->log.at(0) == "init");
        REQUIRE(sut->log.at(1) == "beginrun");
        REQUIRE(sut->log.at(2) == "process");
        REQUIRE(sut->log.at(3) == "process");
        REQUIRE(sut->log.at(4) == "endrun");
        REQUIRE(sut->log.at(5) == "finish");
    }
    SECTION("ConstantRunNumber") {
        app.Add(new SourceWithRunNumberChange);
        app.SetParameterValue("jana:nevents", 2);
        app.Run();
        REQUIRE(sut->log.size() == 6);
        REQUIRE(sut->log.at(0) == "init");
        REQUIRE(sut->log.at(1) == "beginrun");
        REQUIRE(sut->log.at(2) == "process");
        REQUIRE(sut->log.at(3) == "process");
        REQUIRE(sut->log.at(4) == "endrun");
        REQUIRE(sut->log.at(5) == "finish");
    }
    SECTION("MultipleRunNumbers") {
        app.Add(new SourceWithRunNumberChange);
        app.SetParameterValue("jana:nevents", 5);
        app.Run();
        REQUIRE(sut->log.size() == 11);
        REQUIRE(sut->log.at(0) == "init");
        REQUIRE(sut->log.at(1) == "beginrun");
        REQUIRE(sut->log.at(2) == "process");
        REQUIRE(sut->log.at(3) == "process");
        REQUIRE(sut->log.at(4) == "endrun");
        REQUIRE(sut->log.at(5) == "beginrun");
        REQUIRE(sut->log.at(6) == "process");
        REQUIRE(sut->log.at(7) == "process");
        REQUIRE(sut->log.at(8) == "process");
        REQUIRE(sut->log.at(9) == "endrun");
        REQUIRE(sut->log.at(10) == "finish");
    }
}




} // namespace jana::jeventprocessortests
