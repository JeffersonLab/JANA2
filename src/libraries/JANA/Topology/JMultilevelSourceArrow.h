
#include <JANA/Topology/JArrow.h>


class JMultilevelSourceArrow : public JArrow {

private:
    JEventSource* m_source = nullptr;
    std::vector<JEventLevel> m_levels;
    JEventLevel m_child_event_level = JEventLevel::None;
    JEventLevel m_next_input_level;

    std::unordered_map<JEventLevel, std::pair<JEvent*, size_t>> m_pending_parents;
    bool m_finish_in_progress = false;

private:
    void EvictNextParent(OutputData& outputs, size_t& output_count);

public:
    JMultilevelSourceArrow(const std::string& arrow_name, JEventSource* source);
    const std::vector<JEventLevel>& GetLevels() const;

    void Initialize() override;
    void Fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override;
    void Finalize() override;

};

