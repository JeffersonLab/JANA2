
#pragma once
#include <JANA/JService.h>
#include <map>

class TranslationTable_service : public JService {
public:
    struct DAQCoordinates {
        uint32_t crate;
        uint32_t slot;
        uint32_t channel;
    };
    struct DetectorCoordinates {
        uint32_t detector_id;
        uint32_t cell_id;
        std::vector<int> indices;
        double x;
        double y;
        double z;
    };
    struct ChannelCalibration {
        double gain;            // MeV per ADC count
        double pedestal;        // MeV
        double tick_period;     // ns per clock tick
        double time_offset;     // ns
    };

    using DAQLookupTable = std::shared_ptr<std::map<DAQCoordinates, const std::tuple<DetectorCoordinates, ChannelCalibration>>>;

private:

    std::map<int, std::weak_ptr<std::map<DAQCoordinates, const std::tuple<DetectorCoordinates, ChannelCalibration>>>> m_daq_lookups;

    // std::map<int, std::weak_ptr<std::map<uint32_t, const std::tuple<DAQCoordinates, DetectorCoordinates, ChannelCalibration>>>> m_cell_lookups;

    std::vector<std::tuple<int, DAQCoordinates, DetectorCoordinates, ChannelCalibration>> m_hardcoded_data;

public:

    // For hit reconstruction
    DAQLookupTable GetDAQLookupTable(int run_number);

    // For digitization of sim data, as well as two-phase hit reconstruction ("digihits")
    // std::shared_ptr<std::map<uint32_t, const std::tuple<DAQCoordinates, DetectorCoordinates, ChannelCalibration>>> GetCellLookupTable(int run_number);

    // For testing
    void AddHardcodedRow(int run_number, DAQCoordinates daq_coords, DetectorCoordinates det_coords, ChannelCalibration calibs);

private:

    DAQLookupTable FetchDAQLookupTable(int run_number);

    // std::shared_ptr<std::map<uint32_t, const std::tuple<DAQCoordinates, DetectorCoordinates, ChannelCalibration>>> FetchCellLookupTable(int run_number);
};

inline bool operator<(const TranslationTable_service::DAQCoordinates& lhs, const TranslationTable_service::DAQCoordinates& rhs) {
    return (lhs.crate < rhs.crate || lhs.slot < rhs.slot || lhs.channel < rhs.channel);
}


