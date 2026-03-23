
#include <chrono>

class JComponent;
class JArrow;

struct JArrowExecutionInfo {

    using clock_t = std::chrono::steady_clock;

    JArrow* arrow = nullptr;
    std::chrono::time_point<clock_t> start_time;
    std::chrono::time_point<clock_t> finish_time;
};

struct JComponentExecutionInfo {

    enum class ComponentType { None, Factory, Source, Unfolder, Folder, Processor};
    enum class CallbackType { None, Process, ProcessParallel, ProcessSequential, Emit, Unfold, Fold };

    using clock_t = std::chrono::steady_clock;

    std::chrono::time_point<clock_t> start_time;
    std::chrono::time_point<clock_t> finish_time;

    CallbackType callback_type = CallbackType::None;
    ComponentType component_type = ComponentType::None;

    JComponent* component = nullptr;
    JArrowExecutionInfo* arrow_info = nullptr;
};


