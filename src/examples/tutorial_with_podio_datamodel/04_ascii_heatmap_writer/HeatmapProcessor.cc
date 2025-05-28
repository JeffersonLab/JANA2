
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "HeatmapProcessor.h"
#include <jana2_tutorial_podio_datamodel/CalorimeterHitCollection.h>

HeatmapProcessor::HeatmapProcessor() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    SetPrefix("heatmap_writer"); // Used for logger and parameters
    SetCallbackStyle(CallbackStyle::LegacyMode);
}

void HeatmapProcessor::Init() {
    LOG_INFO(GetLogger()) << "HeatmapProcessor::Init: Initializing heatmap";

    m_heatmap = std::make_unique<double[]>(m_cell_cols() * m_cell_rows());

    for (size_t i=0; i < m_cell_rows(); ++i) {
        for (size_t j=0; j < m_cell_cols(); ++j) {
            m_heatmap[i* m_cell_cols() + j] = 0;
        }
    }
}

void HeatmapProcessor::Process(const std::shared_ptr<const JEvent>& event) {
    LOG_INFO(GetLogger()) << "HeatmapProcessor::Process, Event #" << event->GetEventNumber();

    /// Do everything we can in parallel
    /// Warning: We are only allowed to use local variables and `event` here
    auto hits = event->GetCollection<CalorimeterHit>("CalorimeterHit:raw");

    /// Lock mutex
    std::lock_guard<std::mutex>lock(m_mutex);

    /// Do the rest sequentially
    /// Now we are free to access shared state such as m_heatmap
    for (const auto hit : *hits) {
        if (hit.getRow() < m_cell_rows() && hit.getCol() < m_cell_cols()) {
            m_heatmap[hit.getRow() * m_cell_cols() + hit.getCol()] = hit.getEnergy();
        }
        else {
            LOG_WARN(GetLogger()) << "Hit at row=" << hit.getRow() << ", col=" << hit.getCol() << " does not fit on heatmap";
        }
    }
}

void HeatmapProcessor::Finish() {
    // Close any resources
    LOG_INFO(GetLogger()) << "QuickTutorialProcessor::Finish: Displaying heatmap";

    double min_value = m_heatmap[0];
    double max_value = m_heatmap[0];

    for (size_t i=0; i<m_cell_rows(); ++i) {
        for (size_t j=0; j<m_cell_cols(); ++j) {
            double value = m_heatmap[i * m_cell_cols() + j];
            if (min_value > value) min_value = value;
            if (max_value < value) max_value = value;
        }
    }

    std::cout << "  +";
    for (size_t j=0; j<m_cell_cols(); ++j) {
        std::cout << "-";
    }
    std::cout << "+" << std::endl;

    char ramp[] = " .:#";
    for (size_t i=0; i<m_cell_rows(); ++i) {
        std::cout << "  |";
        for (size_t j=0; j<m_cell_cols(); ++j) {
            double value = m_heatmap[i*m_cell_cols() + j];
            int shade = (max_value == min_value) ? 0 : int((value - min_value)/(max_value - min_value) * 3);
            std::cout << ramp[shade];
        }
        std::cout << "|" << std::endl;
    }

    std::cout << "  +";
    for (size_t j=0; j<m_cell_cols(); ++j) {
        std::cout << "-";
    }
    std::cout << "+" << std::endl;
}

