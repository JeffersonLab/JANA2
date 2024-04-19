
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _JEventSourceGenerator_h_
#define _JEventSourceGenerator_h_

#include <string>

#include <JANA/JEventSource.h>

/// This is a base class for all event source generators. JANA implements
/// event sources in a modular way so that new types of sources can be
/// easily added. Typically, this just means different file formats, but
/// can also be networked or shared memory sources.
///
/// One may subclass the JEventSource class directly, but it is recommended
/// to use the template class JEventSourceGeneratorT instead. That defines
/// defaults for the required methods based on the JEventSource class
/// the generator is for while still allowing to customization where
/// needed. See the JEventSourceGeneratorT documentation for details.

class JComponentManager;

class JEventSourceGenerator{
    public:

        friend JComponentManager;

        JEventSourceGenerator(JApplication *app=nullptr):mApplication(app){}
        virtual ~JEventSourceGenerator(){}

        // Default versions of these are defined in JEventSourceGeneratorT.h
        virtual std::string GetType(void) const { return "Unknown";} ///< Return name of the source type this will generate
        virtual std::string GetDescription(void) const { return ""; } ///< Return description of the source type this will generate
        virtual JEventSource* MakeJEventSource( std::string source ) = 0; ///< Create an instance of the source type this generates
        virtual double CheckOpenable( std::string source ) = 0; ///< See JEventSourceGeneratorT for description


    protected:

        /// This is called by JEventSourceManager::AddJEventSourceGenerator which
        /// itself is called by JApplication::Add(JEventSourceGenerator*). There
        /// should be no need to call it from anywhere else.
        void SetJApplication(JApplication *app){ mApplication = app; }

        /// SetPluginName is called by JANA itself and should not be exposed to the user.
        void SetPluginName(std::string plugin_name) { mPluginName = plugin_name; };

        /// GetPluginName is called by JANA itself and should not be exposed to the user.
        std::string GetPluginName() const { return mPluginName; }

        JEventLevel GetLevel() { return mLevel; }
        void SetLevel(JEventLevel level) { mLevel = level; }

        JApplication* mApplication{nullptr};
        std::string mPluginName;
        JEventLevel mLevel = JEventLevel::None;
};

#endif // _JEventSourceGenerator_h_

