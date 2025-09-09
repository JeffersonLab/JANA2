#include "TranslationTable_service.h"



// It is helpful to think of a JService as being a JANA adapter for a third-party data service, meant
// to be used for ANY data that is not inserted into the event by a component ("side-loading").
// In practice this means calibrations, geometry, magnetic field maps, etc.

// We recommend:
// 1. Having JServices be as standalone as possible. In particular, they should not depend on the
//    project's data model, because they are likely to be reused by different projects in the future.
//    For example, if we do a beam test on an ePIC detector in HallD, we would want to reuse the
//    GlueX translation tables with the ePIC data model and reconstruction factories.
//
// 2. Don't include logic that could belong on a factory or algorithm. This is because JServices are effectively 
//    singletons, i.e. there's only one instance in the project at any time. This means that you can't do A/B testing,
//    and can't swap out different implementations like you would with a Factory.
//

void TranslationTable_service::AddHardcodedRow(int run_number,
                                      TranslationTable_service::DAQCoordinates daq_coords,
                                      TranslationTable_service::DetectorCoordinates det_coords,
                                      TranslationTable_service::ChannelCalibration calibs) {

    m_hardcoded_data.push_back({run_number, daq_coords, det_coords, calibs});
}


TranslationTable_service::DAQLookupTable TranslationTable_service::GetDAQLookupTable(int run_number) {

    auto table_it = m_daq_lookups.find(run_number);
    if (table_it != m_daq_lookups.end()) {
        auto locked = table_it->second.lock();
        if (locked) {
            // We have the lookup table still, hand out another copy
            return locked;
        }
    }

    // At this point we either never had the lookup table, or it was already destroyed, so we need to re-fetch it

    auto shared_table = FetchDAQLookupTable(run_number);
    m_daq_lookups[run_number] = shared_table;
    // m_daq_lookups holds on to a weak pointer, which goes away automatically when the last factory lets go
    return shared_table;
}


TranslationTable_service::DAQLookupTable TranslationTable_service::FetchDAQLookupTable(int run_number) {

    // In principle, you could load the table from anywhere you like, including a text file.
    // However, managing external resources is tricky, so we recommend starting with the existing machinery instead of implementing something incrementally.
    // JANA's calibration manager supports both small calibration data (e.g. gain values) and larger resources (e.g. magnetic field maps) with
    // swappable backends including XML, SQLite, and MySQL. If you are working on ePIC, you'll probably be loading these things 
    // through dd4hep instead of JANA2's own calibration manager. It doesn't matter, just be consistent.

    auto shared_table = std::make_shared<std::map<TranslationTable_service::DAQCoordinates,
                                                  const std::tuple<TranslationTable_service::DetectorCoordinates,
                                                                   TranslationTable_service::ChannelCalibration>>>();

    for (auto& hardcoded_row : m_hardcoded_data) {

        auto& row_run_number = std::get<0>(hardcoded_row);
        const DAQCoordinates& daq_coords = std::get<1>(hardcoded_row);
        const DetectorCoordinates& det_coords = std::get<2>(hardcoded_row);
        const ChannelCalibration& calibs = std::get<3>(hardcoded_row);

        if (row_run_number == run_number) {
            shared_table->insert({daq_coords, std::make_tuple(det_coords, calibs)});
        }
    }
    return shared_table;
}

