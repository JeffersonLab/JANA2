
#include "CsvWriter.h"
#include <JANA/JLogger.h>

CsvWriter::CsvWriter() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    SetPrefix("csvwriter");
    SetCallbackStyle(CallbackStyle::ExpertMode);

    m_event_header_in.SetOptional(true);
    m_calo_hit_collections_in.SetRequestedDatabundleNames({"raw"});
}

void CsvWriter::Init() {
    LOG_INFO(GetLogger()) << "Opening output file: " << m_output_filename();
    m_output_file.open(m_output_filename().c_str());
}

void CsvWriter::ProcessSequential(const JEvent& event) {
    LOG << "CsvWriter::Process, Event #" << event.GetEventNumber() << LOG_END;
    m_output_file << "======================" << std::endl;

    for (size_t i=0; i<m_calo_hit_collections_in().size(); ++i) {
        m_output_file << "type_name, databundle_unique_name, count" << std::endl;
        m_output_file << m_calo_hit_collections_in.GetTypeName() << "," << m_calo_hit_collections_in.GetRealizedDatabundleNames().at(i) << ", " << std::endl;
        m_output_file << std::endl;
        m_output_file << "row, col, cell_id, energy, time" << std::endl;
        auto& coll = m_calo_hit_collections_in().at(i);
        for (auto& hit : coll) {
            m_output_file << hit->row << ", " << hit->col << ", " << hit->cell_id << ", " << hit->energy << ", " << hit->time << std::endl;
        }
        m_output_file << std::endl;
    }

}

void CsvWriter::Finish() {
    LOG_INFO(GetLogger()) << "Closing output file: " << m_output_filename();
    m_output_file.close();
}

