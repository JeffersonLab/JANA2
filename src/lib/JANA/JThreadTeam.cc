

#include <JANA/JThreadTeam.h>
#include <JANA/JWorker.h>
#include <JANA/JLogger.h>

using std::string;


JThreadTeam::JThreadTeam(JScheduler& scheduler) : _scheduler(scheduler) {

    logger = JLoggingService::logger("JThreadTeam");
    _status = Status::Idle;
};


JThreadTeam::~JThreadTeam() {
    LOG_DEBUG(logger) << "JThreadTeam destruction started." << LOG_END;
    stop();
    join();
    LOG_DEBUG(logger) << "JThreadTeam destruction finished." << LOG_END;
}


JThreadTeam::Status JThreadTeam::get_status() {
    return _status;
}


std::vector<JWorker::Summary> JThreadTeam::get_worker_summaries() {

    std::vector<JWorker::Summary> summaries;
    for (JWorker* worker : _workers) {
        summaries.push_back(worker->get_summary());
    }
    return summaries;
}


JThreadTeam::Response JThreadTeam::run(int nthreads) {

    LOG_DEBUG(logger) << "JThreadTeam::run() called." << LOG_END;
    if (_status != Status::Idle) {
        return Response::AlreadyRunning;
    }
    _status = Status::Running;
    _workers.reserve(nthreads);

    for (int i = 0; i < nthreads; ++i) {
        JWorker* worker = new JWorker(i, _scheduler);
        _workers.push_back(worker);
    }
    LOG_INFO(logger) << "JThreadTeam status changed to RUNNING." << LOG_END;
    return Response::Success;
}


JThreadTeam::Response JThreadTeam::stop() {
    if (_status == Status::Idle) {
        return Response::NotRunning;
    }
    _status = Status::Stopping;
    bool all_stopped = true;
    for (JWorker* worker : _workers) {
        worker->shutdown_requested = true;
        all_stopped &= worker->shutdown_achieved;
    }
    if (all_stopped) {
        return Response::Success;
    } else {
        return Response::InProgress;
    }
}


JThreadTeam::Response JThreadTeam::join() {

    LOG_DEBUG(logger) << "JThreadTeam::join() called." << LOG_END;
    if (_status == Status::Idle) {
        return Response::NotRunning;
    }

    // Destroying workers will join their respective threads.
    while (_workers.size() > 0) {
        JWorker* worker = _workers.back();
        _workers.pop_back();
        delete worker;
    }

    _status = Status::Idle;
    LOG_INFO(logger) << "JThreadTeam status changed to IDLE" << LOG_END;
    return Response::Success;
}




