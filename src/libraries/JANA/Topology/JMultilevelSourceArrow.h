
#include <JANA/Topology/JMultilevelArrow.h>


class JMultilevelSourceArrow : public JArrow {
public:
    enum class Direction { In, Out };

private:
    std::vector<JEventLevel> m_levels;
    std::map<std::tuple<JEventLevel, Direction>, size_t> m_port_lookup;
    JEventLevel m_child_event_level = JEventLevel::None;
    JEventLevel m_next_input_level;

    bool m_finish_in_progress = false;
    std::vector<JEvent*> m_pending_outputs;

    std::unordered_map<JEventLevel, std::pair<JEvent*, size_t>> m_pending_parents;
    size_t m_total_emitted_event_count = 0;


public:
    const std::vector<JEventLevel>& GetLevels() const;
    size_t GetPortIndex(JEventLevel level, Direction direction) const;

    void SetLevels(std::vector<JEventLevel> levels);


    void initialize() override;
    void EvictParent(JEventLevel level, OutputData& outputs);
    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override;
    virtual JEventSource::Result DoNext(JEvent& event);
    void finalize() override;

};

