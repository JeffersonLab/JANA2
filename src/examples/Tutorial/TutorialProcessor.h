
#ifndef _TutorialProcessor_h_
#define _TutorialProcessor_h_

#include <JANA/JEventProcessor.h>

class TutorialProcessor : public JEventProcessor {

    double m_heatmap[100][100];
    // Shared state (e.g. histograms, TTrees, TFiles) live
    std::mutex m_mutex;
    
public:

    TutorialProcessor();
    virtual ~TutorialProcessor() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};


#endif // _TutorialProcessor_h_

