
#include <JOmniFactory.h>


template <typename LevelT> 
struct JOmniProcessor : public HasProcessFn<LevelT> {

    std::mutex m_mutex;
};
