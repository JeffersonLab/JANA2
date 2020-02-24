
#include "TutorialProcessor.h"
#include "Hit.h"
#include <JANA/JLogger.h>

TutorialProcessor::TutorialProcessor() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}

void TutorialProcessor::Init() {
    LOG << "QuickTutorialProcessor::Init: Initializing heatmap" << LOG_END;

    for (int i=0; i<100; ++i) {
        for (int j=0; j<100; ++j) {
            m_heatmap[i][j] = 0.0;
        }
    }
}

void TutorialProcessor::Process(const std::shared_ptr<const JEvent> &event) {
    LOG << "TutorialProcessor::Process, Event #" << event->GetEventNumber() << LOG_END;
    
    /// Do everything we can in parallel
    /// Warning: We are only allowed to use local variables and `event` here
    auto hits = event->Get<Hit>();

    /// Lock mutex
    std::lock_guard<std::mutex>lock(m_mutex);

    /// Do the rest sequentially
    /// Now we are free to access shared state such as m_heatmap
    for (const Hit* hit : hits) {
        m_heatmap[hit->x][hit->y] += hit->E;    // ADD ME
        /// Update shared state
    }
}

void TutorialProcessor::Finish() {
    // Close any resources
    LOG << "QuickTutorialProcessor::Finish: Displaying heatmap" << LOG_END;

    double min_value = m_heatmap[0][0];
    double max_value = m_heatmap[0][0];

    for (int i=0; i<100; ++i) {
        for (int j=0; j<100; ++j) {
            double value = m_heatmap[i][j];
            if (min_value > value) min_value = value;
            if (max_value < value) max_value = value;
        }
    }
    if (min_value != max_value) {
        char ramp[] = " .:-=+*#%@";
        for (int i=0; i<100; ++i) {
            for (int j=0; j<100; ++j) {
                int shade = int((m_heatmap[i][j] - min_value)/(max_value - min_value) * 9);
                std::cout << ramp[shade];
            }
            std::cout << std::endl;
        }
    }
}

