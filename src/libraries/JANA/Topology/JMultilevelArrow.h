
// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Topology/JArrow.h>


class JMultilevelArrow : public JArrow {
public:
    enum class Direction { In, Out };
    enum class Style { AllToAll, AllToOne, OneToAll };

private:
    Style m_style = Style::AllToAll;
    std::vector<JEventLevel> m_levels;
    std::map<std::tuple<JEventLevel, Direction>, size_t> m_port_lookup;
    JEventLevel m_next_input_level;
    std::vector<JEvent*> m_pending_outputs;
    FireResult m_pending_fireresult = FireResult::KeepGoing;

public:
    JMultilevelArrow() = default;
    virtual ~JMultilevelArrow() = default;
    void ConfigurePorts(Style style, std::vector<JEventLevel> levels);
    size_t GetPortIndex(JEventLevel level, Direction direction) const;
    const std::vector<JEventLevel>& GetLevels() const;
    void initialize() override;
    void finalize() override;
    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) override;
    virtual void Process(JEvent* input, std::vector<JEvent*>& outputs, JEventLevel& next_input_level, JArrow::FireResult& result);
};


