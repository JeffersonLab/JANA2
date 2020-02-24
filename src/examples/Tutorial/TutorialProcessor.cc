
#include "TutorialProcessor.h"
#include <JANA/JLogger.h>

TutorialProcessor::TutorialProcessor() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}

void TutorialProcessor::Init() {
    LOG << "TutorialProcessor::Init" << LOG_END;
    // Open TFiles, set up TTree branches, etc
}

void TutorialProcessor::Process(const std::shared_ptr<const JEvent> &event) {
    LOG << "TutorialProcessor::Process, Event #" << event->GetEventNumber() << LOG_END;
    
    /// Do everything we can in parallel
    /// Warning: We are only allowed to use local variables and `event` here
    //auto hits = event->Get<Hit>();

    /// Lock mutex
    std::lock_guard<std::mutex>lock(m_mutex);

    /// Do the rest sequentially
    /// Now we are free to access shared state such as m_heatmap
    //for (const Hit* hit : hits) {
        /// Update shared state
    //}
}

void TutorialProcessor::Finish() {
    // Close any resources
    LOG << "TutorialProcessor::Finish" << LOG_END;
}

