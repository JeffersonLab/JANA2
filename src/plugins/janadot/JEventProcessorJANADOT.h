// Author: David Lawrence  August 8, 2007
//
//

#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>
using namespace jana;

#include <map>
using std::map;

class JEventProcessorJANADOT:public JEventProcessor
{

	/// The janadot plugin is used to generate a call graph with information on
	/// factory dependencies on one another as well as some profiling to indicate
	/// where the processing time is being spent. The output is a text file containing
	/// commands in GraphVix's "DOT" language. This can be converted into a .ps file using
	/// the "dot" program (usually installed with the "graphviz" package on Linux.) The
	/// .ps file can then be converted to PDF using ps2pdf.if
	/// There are a couple of options in janadot that can be controlled via configuration
	/// parameters. It should be noted that janadot itself turns on the RECORD_CALL_STACK
	/// configuration parameter so that the framework will record the information needed
	/// to produce the call graph in the end. Configuration parameters specific to [janadot
	/// are:
	/// 
	/// JANADOT:GROUP:GroupName  - This is used to tell janadot to issue commands that will
	/// cause dot to draw the graph using colors and/or a bounding box to identify members
	/// of a group. 
	/// This is useful for larger projects with lots of factories. The value of
	/// "GroupName" is used for the title when drawing a bounding box.
	/// The value of the configuration parameter
	/// should be a comma separated list of factories to include in the group. If a
	/// factory with a name matching "color_XXX" is found in the factory list, then the
	/// XXX part is used as the color for the background of the classes in the group.
	/// For example:
	/// 
	/// -PPLUGINS=janadot -PJANADOT:GROUP:Tracking=DTrackHit,DTrackCandidate,DTrackWireBased,color_red
	/// 
	/// This will cause dot to draw the boxes for DTrackHit,DTrackCandidate, and
	/// DTrackWireBased near each other, with a red background then a larger box drawn
	/// around it. A label that says "Tracking" will also be draw at the top of the
	/// bounding box.
	///
	/// If one does not wish to have the bounding and label box drawn then a value
	/// of "no_box" should be added to the list of values (similar to how "color_red"
	/// was in the above example). Note that these special keyward can be contained
	/// anywhere in the list and need not be at the end.
	/// 
	/// JANADOT:SUPPRESS_UNUSED_FACTORIES  - By default this is set to true, but one can
	/// turn it off by setting it to "0". This only has an effect if a JANADOT:GROUP:
	/// config. parameter (described above) is given. If this is turned off AND a
	/// factory is specified in the list of factories for the group that does not
	/// get called during processing, then an ellipse for that factory will be drawn
	/// inside the bounding box for the group. These types of factories will have
	/// no connections to them and will be filled with white so they will look a
	/// little ghostly. In most instances, you will not want this behavior so it
	/// is supressed by default.
	///
	/// JANADOT:FOCUS  - If this is set then the value is taken to be the factory
	/// on which the graph should focus. Specifically, any factories that are not
	/// direct decendents or ancestors of the focus factory are ommitted from the
	/// graph. The times and percentages drawn are the same as for the full graph
	/// no not all time accounting will be visible on such graphs. The specified
	/// focus factory is drawn with a triple octagon shape to indicate it was used
	/// as the focus.
	///
	/// Because the configurations can become large for large projects, a script
	/// called "janadot_groups.py" is provided as part of JANA. Just give it the
	/// path of the top level directory structure where code you wish to document
	/// it is.


	public:
		const char* className(void){return "JEventProcessorJANADOT";}

		jerror_t init(void);							///< Called once at program start.
		jerror_t brun(JEventLoop *loop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *loop, uint64_t eventnumber);	///< Called every event.
		jerror_t erun(void){return NOERROR;};	///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);							///< Called after last event of last event source has been processed.

		enum node_type{
			kDefault,
			kProcessor,
			kFactory,
			kSource
		};

		class CallLink{
			public:
				string caller_name;
				string caller_tag;
				string callee_name;
				string callee_tag;
				
				bool operator<(const CallLink &link) const {
					if(this->caller_name!=link.caller_name)return this->caller_name < link.caller_name;
					if(this->callee_name!=link.callee_name)return this->callee_name < link.callee_name;
					if(this->caller_tag!=link.caller_tag)return this->caller_tag < link.caller_tag;
					return this->callee_tag < link.callee_tag;
				}
		};
		
		class CallStats{
			public:
				CallStats(void){
					from_cache_ms = 0;
					from_source_ms = 0;
					from_factory_ms = 0;
					data_not_available_ms = 0;
					Nfrom_cache = 0;
					Nfrom_source = 0;
					Nfrom_factory = 0;
					Ndata_not_available = 0;
				}
				double from_cache_ms;
				double from_source_ms;
				double from_factory_ms;
				double data_not_available_ms;
				unsigned int Nfrom_cache;
				unsigned int Nfrom_source;
				unsigned int Nfrom_factory;
				unsigned int Ndata_not_available;
		};
		
		class FactoryCallStats{
			public:
				FactoryCallStats(void){
					type = kDefault;
					time_waited_on = 0.0;
					time_waiting = 0.0;
					Nfrom_factory = 0;
					Nfrom_source = 0;
					Nfrom_cache = 0;
				}
				node_type type;
				double time_waited_on;	// time other factories spent waiting on this factory
				double time_waiting;		// time this factory spent waiting on other factories
				unsigned int Nfrom_factory;
				unsigned int Nfrom_source;
				unsigned int Nfrom_cache;
		};

		map<CallLink, CallStats> call_links;
		map<string, FactoryCallStats> factory_stats;
		pthread_mutex_t mutex;
		bool force_all_factories_active;
		bool suppress_unused_factories;
		map<string,vector<string> > groups;
		map<string,string > group_colors;
		map<string,string > node_colors;
		set<string> no_subgraph_groups;
		bool has_focus;
		string focus_factory;
		
		void FindDecendents(string caller, set<string> &decendents);
		void FindAncestors(string callee, set<string> &ancestors);
		string MakeTimeString(double time_in_ms);
		string MakeNametag(const string &name, const string &tag);
};
