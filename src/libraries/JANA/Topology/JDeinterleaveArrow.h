
#include <JANA/Topology/JMultilevelArrow.h>


class JDeinterleaveArrow : public JMultilevelArrow {
private:
    std::unordered_map<JEventLevel, JEvent*> m_pending_parents;
    JEventLevel m_child_event_level = JEventLevel::None;

public:

    JDeinterleaveArrow();

    virtual ~JDeinterleaveArrow();

    void SetLevels(std::vector<JEventLevel> levels);

    void Process(JEvent* input, std::vector<JEvent*>& outputs, JEventLevel& next_input_level, JArrow::FireResult& result) override;

};

