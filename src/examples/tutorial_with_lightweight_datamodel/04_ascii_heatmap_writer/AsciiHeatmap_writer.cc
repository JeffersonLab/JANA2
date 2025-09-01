
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "AsciiHeatmap_writer.h"
#include "CalorimeterHit.h"

AsciiHeatmap_writer::AsciiHeatmap_writer() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    SetPrefix("ascii_heatmap_writer");    // Used for logger and parameters
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

void AsciiHeatmap_writer::Init() {
    LOG_INFO(GetLogger()) << "AsciiHeatmap_writer::Init: Initializing heatmap";

    m_heatmap = std::make_unique<double[]>(m_cell_cols() * m_cell_rows());

    for (size_t i=0; i < m_cell_rows(); ++i) {
        for (size_t j=0; j < m_cell_cols(); ++j) {
            m_heatmap[i* m_cell_cols() + j] = 0;
        }
    }
}

void AsciiHeatmap_writer::ProcessSequential(const JEvent& event) {
    LOG_INFO(GetLogger()) << "AsciiHeatmap_writer::Process, Event #" << event.GetEventNumber();

    for (const CalorimeterHit* hit : m_hits_in()) {
        if (hit->row < (int) m_cell_rows() && hit->col < (int) m_cell_cols()) {
            m_heatmap[hit->row* m_cell_cols() + hit->col] = hit->energy;
        }
        else {
            LOG_WARN(GetLogger()) << "Hit at row=" << hit->row << ", col=" << hit->col << " does not fit on heatmap";
        }
    }
}

void AsciiHeatmap_writer::Finish() {
    // Close any resources
    LOG_INFO(GetLogger()) << "AsciiHeatmap_writer::Finish: Displaying heatmap";

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

