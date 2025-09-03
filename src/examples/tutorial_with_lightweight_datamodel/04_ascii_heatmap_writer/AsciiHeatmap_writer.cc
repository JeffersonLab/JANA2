
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "AsciiHeatmap_writer.h"
#include "CalorimeterHit.h"
#include <string>

AsciiHeatmap_writer::AsciiHeatmap_writer() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    SetPrefix("ascii_heatmap_writer");    // Used for logger and parameters
    SetCallbackStyle(CallbackStyle::ExpertMode);

    // Detector dimensions are shared with RandomHitSource
    m_cell_cols.SetShared(true);
    m_cell_rows.SetShared(true);

    m_hits_in.SetTag("rechits");
}

void AsciiHeatmap_writer::Init() {
    LOG_INFO(GetLogger()) << "AsciiHeatmap_writer::Init: Initializing heatmap";
    m_heatmap.resize(m_cell_cols() * m_cell_rows());

    if (*m_use_unicode) {
        m_ramp = {" ", "░", "▒", "▓", "█"};
        m_box_topleft = "┌";
        m_box_topright = "┐";
        m_box_bottomleft = "└";
        m_box_bottomright = "┘";
        m_box_vertical = "│";
        m_box_horizontal = "─";
    }
    else {
        m_ramp = {" ", ".", ":", "%", "#"};
        m_box_topleft = "+";
        m_box_topright = "+";
        m_box_bottomleft = "+";
        m_box_bottomright = "+";
        m_box_vertical = "|";
        m_box_horizontal = "-";
    }
}

void AsciiHeatmap_writer::ProcessSequential(const JEvent& event) {
    LOG_INFO(GetLogger()) << "AsciiHeatmap_writer::Process, Event #" << event.GetEventNumber();

    // Clear heatmap
    for (size_t i=0; i < m_cell_rows(); ++i) {
        for (size_t j=0; j < m_cell_cols(); ++j) {
            m_heatmap[i* m_cell_cols() + j] = 0;
        }
    }

    // Populate heatmap with sum of hit values
    for (const CalorimeterHit* hit : m_hits_in()) {
        if (hit->row < (int) m_cell_rows() && hit->col < (int) m_cell_cols()) {
            m_heatmap[hit->row* m_cell_cols() + hit->col] += hit->energy;
        }
        else {
            LOG_WARN(GetLogger()) << "Hit at row=" << hit->row << ", col=" << hit->col << " does not fit on heatmap";
        }
    }

    // Find min and max values
    double min_value = m_heatmap[0];
    double max_value = m_heatmap[0];

    for (size_t i=0; i<m_cell_rows(); ++i) {
        for (size_t j=0; j<m_cell_cols(); ++j) {
            double value = m_heatmap[i * m_cell_cols() + j];
            if (min_value > value) min_value = value;
            if (max_value < value) max_value = value;
        }
    }

    // Show heatmap
    std::cout << "Event: " << event.GetEventNumber() << std::endl;
    std::cout << "  " << m_box_topleft;
    for (size_t j=0; j<m_cell_cols(); ++j) {
        std::cout << m_box_horizontal << m_box_horizontal;
    }
    std::cout << m_box_topright << std::endl;


    for (size_t i=0; i<m_cell_rows(); ++i) {
        std::cout << "  " << m_box_vertical;
        for (size_t j=0; j<m_cell_cols(); ++j) {
            double value = m_heatmap[i*m_cell_cols() + j];
            int shade = (max_value == min_value) ? 0 : int((value - min_value)/(max_value - min_value) * 4);
            std::cout << m_ramp[shade] << m_ramp[shade]; // Double up so that cells look square
        }
        std::cout << m_box_vertical << std::endl;
    }

    std::cout << "  " << m_box_bottomleft;
    for (size_t j=0; j<m_cell_cols(); ++j) {
        std::cout << m_box_horizontal << m_box_horizontal;
    }
    std::cout << m_box_bottomright << std::endl;

}

void AsciiHeatmap_writer::Finish() {
    // Close any resources, write any files
    LOG_INFO(GetLogger()) << "AsciiHeatmap_writer::Finish";
}

