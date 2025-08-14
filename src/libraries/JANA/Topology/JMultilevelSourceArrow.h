
#include <JANA/Topology/JArrow.h>


class JMultilevelSourceArrow : public JArrow {
public:
    enum class Direction { In, Out };

private:
    JEventSource* m_source = nullptr;

    std::vector<JEventLevel> m_levels;
    std::map<std::tuple<JEventLevel, Direction>, size_t> m_port_lookup;
    JEventLevel m_child_event_level = JEventLevel::None;
    JEventLevel m_next_input_level;

    std::unordered_map<JEventLevel, std::pair<JEvent*, size_t>> m_pending_parents;
    bool m_finish_in_progress = false;

private:
    void EvictNextParent(OutputData& outputs, size_t& output_count);

public:
    const std::vector<JEventLevel>& GetLevels() const;
    size_t GetPortIndex(JEventLevel level, Direction direction) const;

    void SetEventSource(JEventSource* source);

    void initialize() override;
    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override;
    void finalize() override;

};

