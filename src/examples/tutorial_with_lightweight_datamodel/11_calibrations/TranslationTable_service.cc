#include "TranslationTable_service.h"

void TranslationTable_service::Init() {
}

void TranslationTable_service::AddRow(int run_number,
                                      TranslationTable_service::DAQCoordinates daq_coords,
                                      TranslationTable_service::DetectorCoordinates det_coords) {

    std::map<DAQCoordinates, DetectorCoordinates>* table = nullptr;
    auto table_it = m_daq_to_det_lookup.find(run_number);
    if (table_it == m_daq_to_det_lookup.end()) {
        auto owned_table = std::make_unique<std::map<DAQCoordinates, DetectorCoordinates>>();
        table = owned_table.get();
        m_daq_to_det_lookup[run_number] = std::move(owned_table);
    }
    else {
        table = table_it->second.get();
    }
    (*table)[daq_coords] = det_coords;
}

const TranslationTable_service::DetectorCoordinates& TranslationTable_service::TranslateDAQCoordinates(int run_number,
                                                                                                       const DAQCoordinates& daq_coords) {
    std::map<DAQCoordinates, DetectorCoordinates>* table = nullptr;
    auto table_it = m_daq_to_det_lookup.find(run_number);
    if (table_it == m_daq_to_det_lookup.end()) {
        auto owned_table = FetchTable(run_number);
        table = owned_table.get();
        m_daq_to_det_lookup[run_number] = std::move(owned_table);
    }
    else {
        table = table_it->second.get();
    }
    return table->at(daq_coords);
}

std::unique_ptr<std::map<TranslationTable_service::DAQCoordinates,
                         TranslationTable_service::DetectorCoordinates>> TranslationTable_service::FetchTable(int /*run_number*/) {
    throw JException("FetchTable() not implemented yet! Use hardcoded table instead. (Hint: Are you using a different run number from what you hardcoded?)");
}
