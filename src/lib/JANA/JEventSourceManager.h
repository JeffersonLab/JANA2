#ifndef JEventSourceManager_h
#define JEventSourceManager_h

#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include <cstddef>

#include "JApplication.h"
#include "JEventSource.h"
#include "JEventSourceGenerator.h"

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JEventSourceManager;

/**************************************************************** TYPE DEFINITIONS ****************************************************************/

class JEventSourceManager
{
	public:

		//STRUCTORS
		JEventSourceManager(JApplication* aApp);

		void AddEventSource(const std::string& source_name);
		void AddJEventSource(JEventSource *source);
		void AddJEventSourceGenerator(JEventSourceGenerator *source_generator);

		void GetJEventSources(std::vector<JEventSource*>& aSources) const;
		void GetJEventSourceGenerators(std::vector<JEventSourceGenerator*>& aGenerators) const;

		void OpenInitSources(void); //Call while single-threaded!!

		//INFORMATION
		bool AreAllFilesClosed(void) const;
		std::size_t GetNumEventsProcessed(void) const;

		void ClearJEventSourceGenerators(void);
		void RemoveJEventSource(JEventSource *source);
		void RemoveJEventSourceGenerator(JEventSourceGenerator *source_generator);

		std::pair<JEventSource::RETURN_STATUS, JEventSource*> OpenNext(const JEventSource* aPreviousSource);

	private:

		JEventSourceGenerator* GetUserEventSourceGenerator(void);
		JEventSourceGenerator* GetEventSourceGenerator(const std::string& source_name);

		JEventSource* OpenSource(const std::string& source_name);

		JApplication* mApplication;
		std::size_t mMaxNumOpenFiles;

		std::vector<std::string> _source_names;
		std::deque<std::string> _source_names_unopened;

		std::vector<JEventSourceGenerator*> _eventSourceGenerators;
		std::vector<JEventSource*> _sources_active;
		std::vector<JEventSource*> _sources_exhausted;

		mutable std::mutex mSourcesMutex;
};

/**************************************************************** MEMBER FUNCTION DEFINITIONS ****************************************************************/

#endif
