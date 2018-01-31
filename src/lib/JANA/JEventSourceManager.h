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

		void GetActiveJEventSources(std::vector<JEventSource*>& aSources) const;
		void GetUnopenedJEventSources(std::deque<JEventSource*>& aSources) const;
		void GetJEventSourceGenerators(std::vector<JEventSourceGenerator*>& aGenerators) const;

		void CreateSources(void); //Call while single-threaded!!
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

		JEventSource* CreateSource(const std::string& source_name);

		JApplication* mApplication = nullptr;
		std::size_t mMaxNumOpenFiles = 2; //TODO: Make configurable!

		std::vector<std::string> _source_names;

		std::vector<JEventSourceGenerator*> _eventSourceGenerators;
		std::deque<JEventSource*> _sources_unopened;
		std::vector<JEventSource*> _sources_active;
		std::vector<JEventSource*> _sources_exhausted;

		mutable std::mutex mSourcesMutex;
};

/**************************************************************** MEMBER FUNCTION DEFINITIONS ****************************************************************/

#endif
