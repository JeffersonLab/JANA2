#ifndef JTestEventProcessor_h
#define JTestEventProcessor_h

#include <atomic>
#include <iostream>
#include <algorithm>

#include <JANA/JApplication.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JTestEventProcessor : public JEventProcessor{

	public:

		JTestEventProcessor(JApplication* app) : JEventProcessor(app) {}

		~JTestEventProcessor() {
			std::cout << "Total # objects = " << mNumObjects <<  std::endl;
		}

		const char* className() {
			return "JTestEventProcessor";
		}

		void Init() {
			std::cout << "JTestEventProcessor::Init() called" << std::endl;

            japp->GetJParameterManager()->SetDefaultParameter(
                    "jtest:min_bytes_per_event",
                    min_bytes,
                    "Bytes per event");

            japp->GetJParameterManager()->SetDefaultParameter(
                    "jtest:max_bytes_per_event",
                    max_bytes,
                    "Bytes per event");
		}

		void Process(const std::shared_ptr<const JEvent>& aEvent) {

		    std::vector<char> buffer;
		    size_t event_buffer_size = randint(min_bytes, max_bytes);
		    mNumObjects += event_buffer_size;

		    for (int i=0; i<event_buffer_size; ++i) {
		        buffer.push_back(2);
		    }
		    long sum = 0;
            for (int i=0; i<event_buffer_size; ++i) {
                sum += buffer[i];
            }
            std::cout << "Processed " << aEvent->GetEventNumber() << " => " << sum/2 << std::endl;
		}

		void Finish() {
			std::cout << "JTestEventProcessor::Finish() called" << std::endl;
		}

	private:
		std::atomic<std::size_t> mNumObjects{0};
		size_t min_bytes = 100000;
		size_t max_bytes = 2100000;
};

#endif // JTestEventProcessor

