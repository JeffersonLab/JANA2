
#pragma once
#include <JANA/JService.h>
#include <map>

class TranslationTable_service : public JService {
public:
    struct DAQCoordinates {
        int crate;
        int slot;
        int channel;
    };
    struct DetectorCoordinates {
        int detector_id;
        int cell_id;
        std::vector<int> indices;
        double x;
        double y;
        double z;
    };

private:

    std::map<int, std::unique_ptr<std::map<DAQCoordinates, DetectorCoordinates>>> m_daq_to_det_lookup;
    // std::map<int, DAQCoordinates> m_cell_id_to_daq_lookup;
    // std::map<int, DetectorCoordinates> m_cell_id_to_det_lookup;

public:
    void Init() override;
    const DetectorCoordinates& TranslateDAQCoordinates(int run_number, const DAQCoordinates&);
    // const DetectorCoordinates& Translate(int run_number, const DAQCoordinates&) const;
    void AddRow(int run_number, DAQCoordinates daq_coords, DetectorCoordinates det_coords);

    std::unique_ptr<std::map<DAQCoordinates, DetectorCoordinates>> FetchTable(int run_number);
    
};

inline bool operator<(const TranslationTable_service::DAQCoordinates& lhs, const TranslationTable_service::DAQCoordinates& rhs) {
    return (lhs.crate < rhs.crate || lhs.slot < rhs.slot || lhs.channel < rhs.channel);
}
