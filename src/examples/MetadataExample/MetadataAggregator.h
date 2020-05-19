
#ifndef _MetadataExampleProcessor_h_
#define _MetadataExampleProcessor_h_

#include <JANA/JEventProcessor.h>
#include <map>



class MetadataAggregator : public JEventProcessor {

    /// Container for all of the statistics we want to collect from our event stream
    struct Statistics {
        size_t event_count = 0;
        size_t total_track_count = 0;
        std::chrono::nanoseconds total_latency_ns {0};
    };

    std::mutex m_mutex;

    std::string m_track_factory = "smeared"; // So we can choose what we are measuring at runtime

    std::map<int, Statistics> m_statistics; // Keyed off of run nr

    // Note that just like with JObjects, we don't want to store pointers to JMetadata here, because
    // they will be overwritten when JEvents get recycled and dangle when JEvents are destroyed.
    // Instead, we may either copy data out, or copy the entire structure outright, which is
    // cheap when the JMetadata object is TriviallyCopyable, which we obviously encourage.

    // We are also going to cache the last entry in our map so that we only have to do a map lookup
    // when the run number changes.

    int m_last_run_nr = -1;
    Statistics* m_last_statistics = nullptr;


public:

    MetadataAggregator();
    virtual ~MetadataAggregator() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};


#endif // _MetadataExampleProcessor_h_

