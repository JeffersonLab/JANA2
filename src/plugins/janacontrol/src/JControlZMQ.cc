
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/kern_return.h>
#include <mach/host_info.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif // __APPLE__

#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <iterator>
#include <set>
#include <mutex>
#include <cmath>
using namespace std;

#include <JANA/JLogger.h>

#include "JControlZMQ.h"
#include "JControlEventProcessor.h"
#include "janaJSON.h"


// The following are convenience macros that make
// the code below a little more succinct and easier
// to read. It changes this:
//
//  JSONADD(key) << val;
//
//   to
//
//  ss<<"\n,\""<<key<<"\":" << val;
//
// The JSONADS version takes a second argument and
// encapsulates it in double quotes in the JSON output.
//#define JSONADD(K) ss<<",\n\""<<K<<"\":"
//#define JSONADS(K,V) ss<<",\n\""<<K<<"\":\""<<V<<"\""


template<class T>
void JSONADD(stringstream &ss, string K, T V, int indent_level=0, bool first_element=false){
        if( ! first_element ) ss << ",\n";
        ss << string(indent_level*2, ' ');
        ss << "\"" << K << "\":";
        ss << "\"" << V << "\"";
}
void JSONOPENARRAY(stringstream &ss, string K, int indent_level=0, bool first_element=false){
    if( ! first_element ) ss << ",\n";
    ss << string(indent_level*2, ' ');
    ss << "\"" << K << "\":";
    ss << "[\n";
}
void JSONCLOSEARRAY(stringstream &ss, int indent_level=0){
    ss << "\n" << string(indent_level*2, ' ') + "]";
}
void JSONOPENSECTION(stringstream &ss, int indent_level=0, bool first_element=false){
    if( ! first_element ) ss << ",\n";
    ss << string(indent_level*2, ' ');
    ss << "{\n";
}
void JSONCLOSESECTION(stringstream &ss, int indent_level=0){
    ss << "\n" << string(indent_level*2, ' ') + "}";
}

//-------------------------------------------------------------
// JControlZMQ
//-------------------------------------------------------------
JControlZMQ::JControlZMQ(JApplication *app, int port):_japp(app), _port(port)
{
    // This is called from jcontrol plugin's InitPlugin() routine

    LOG << "Launching JControlZMQ thread ..." << LOG_END

    // Create new zmq context, get the current host name, and launch server thread
    _zmq_context = zmq_ctx_new();
    gethostname( _host, 256 );
    _pid = getpid();
    _thr = new std::thread( &JControlZMQ::ServerLoop, this );

    // Create new JControlEventProcessor and add to application.
    // This is used to access event info via JEvent. It is also
    // used when in debug_mode to step through events one at a
    // time.
    _jproc = new JControlEventProcessor(_japp);
    _japp->Add(_jproc);
}

//-------------------------------------------------------------
// ~JControlZMQ
//-------------------------------------------------------------
JControlZMQ::~JControlZMQ()
{
    // Tell server thread to quit and wait for it to finish
    _done =true;
    if(_thr != nullptr) {
        LOG << "Joining JControlZMQ thread ..." << LOG_END
        _thr->join();
        delete _thr;
        LOG << "JControlZMQ thread joined." << LOG_END
    }

    // Delete the zmq context
    if( _zmq_context != nullptr ) zmq_ctx_destroy( _zmq_context );

}

//-------------------------------------------------------------
// ServerLoop
//-------------------------------------------------------------
void JControlZMQ::ServerLoop()
{
    /// This implements the zmq server for handling REQ-REP requests.
    /// It is run in a separate thread and should generally take very
    /// little CPU unless it gets pounded by requests. It will loop
    /// indefinitely until the internal _done member is set to true.
    /// Currently, that is done only by calling the destructor.

    LOG << "JControlZMQ server starting ..." << LOG_END

    // This just makes it easier to identify this thread while debugging.
    // Linux and Mac OS X use different calling signatures for pthread_setname_np
#if __linux__
    pthread_setname_np( pthread_self(), "JControlZMQ::ServerLoop" );
#elif defined(__APPLE__)
    pthread_setname_np( "JControlZMQ::ServerLoop" );
#endif

    // Bind to port number specified in constructor. Most likely this came from JANA_ZMQ_PORT config. parameter
    char bind_str[256];
	sprintf( bind_str, "tcp://*:%d", _port );
	void *responder = zmq_socket( _zmq_context, ZMQ_REP );
	auto ret = zmq_bind( responder, bind_str);
	if( ret != 0 ){
		LOG << "JControlZMQ: Unable to bind zeroMQ control socket " << _port << "!" << LOG_END
		perror("zmq_bind");
		return;
	}

	// Loop until told to quit
	while( !_done ){

		// Listen for request (non-blocking)
		char buff[2048];
		auto rc = zmq_recv( responder, buff, 2048, ZMQ_DONTWAIT);
		if( rc< 0 ){
			if( (errno==EAGAIN) || (errno==EINTR) ){
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
				continue;
			}else{
				LOG << "JControlZMQ: ERROR listening on control socket: errno=" << errno << LOG_END
				_done = true;
				continue;
			}
		}

		// Split request into tokens
		std::vector<string> vals;
		istringstream iss( std::string(buff, rc) );
		copy( istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(vals) );

		// Response is JSON that gets built up in ss
		stringstream ss;

		// Add content based on command (i.e. vals[0])
		if( vals.empty()){
		    //------------------ Empty Command
			ss << "{message:\"Empty Command\"}";
		}else if( vals[0] == "quit" ){
		    //------------------ quit
		    _done = true;
		    if( vals.size()>1 )  _japp->SetExitCode( strtol(vals[1].c_str(), nullptr, 10) ); // allow remote user to optionally set exit code.
            _japp->Quit();
			ss << "{message:\"OK\"}";
		}else if( vals[0] == "get_status" ){
		    //------------------ get_status
			ss << GetStatusJSON();
		}else if( vals[0]=="set_nthreads" ){
			//------------------ set_nthreads
			if( vals.size()==2 ){
				int Nthreads = strtol( vals[1].c_str(), nullptr, 10 );
				_japp->Scale( Nthreads );
				ss << "OK";
			}else{
				ss << "wrong number of args to set_nthreads. 1 expected, " << vals.size() << " received";
			}
        }else if( vals[0]=="decrement_nthreads" ){
            //------------------ decrement_nthreads
            auto Nthreads = _japp->GetNThreads();
            Nthreads--;
            if(Nthreads>0) {
                _japp->Scale(Nthreads);
                ss << "OK";
            }else{
                ss << "WARNING: Will not decrement to <1 thread";
            }
        }else if( vals[0]=="increment_nthreads" ){
            //------------------ increment_nthreads
            auto Nthreads = _japp->GetNThreads();
            Nthreads++;
            _japp->Scale( Nthreads );
            ss << "OK";
		}else if( vals[0] == "get_file_size" ){ // mulitple files may be specified
		    //------------------ get_file_size
			if( vals.size()<2){
				ss << "{message:\"ERROR: No file given!\"}";
			}else{
			    ss << "{";
				for( uint32_t i=1; i<vals.size(); i++){

					auto &fname = vals[i];
					struct stat st ={};
					int64_t fsize = -1;
					if( stat(fname.c_str(), &st) == 0) fsize = (int64_t)st.st_size;
					if( i>1 ) ss << ",";
					ss << "\"" << fname << "\"" << ":" << fsize << "\n";
				}
				ss << "}";
			}
        }else if( vals[0] == "get_disk_space" ){ // mulitple directories may be specified
            //------------------ get_disk_space
            if( vals.size()<2){
                ss << "{message:\"ERROR: No directory given!\"}";
            }else{
                ss << "{";
                for( uint32_t i=1; i<vals.size(); i++){

                    auto &dname = vals[i];

                    std::map<std::string,float> myvals;
                    GetDiskSpace( dname, myvals);
                    for( const auto &p : myvals ) ss << "\"" << p.first << "\":" << p.second << "\n";
                }
                ss << "}";
            }
        }else if( vals[0] == "get_factory_list" ){
            //------------------ get_factory_list
            ss << GetJANAFactoryListJSON();
        }else if( vals[0]=="stop" ){
            //------------------ stop
            _japp->Stop();
            ss << "OK";
        }else if( vals[0]=="resume" ){
            //------------------ resume
            _japp->Resume();
            ss << "OK";
        }else if( vals[0]=="debug_mode" ){
            //------------------ debug_mode
            if( vals.size()==2 ){
                bool debug_mode = strtol( vals[1].c_str(), nullptr, 10 ) != 0;
                _jproc->SetDebugMode(debug_mode);
                ss << "OK";
            }else{
                ss << "wrong number of args to debug_mode. 1 expected, " << vals.size() << " received";
            }
        }else if( vals[0]=="next_event" ){
            //------------------ next_event
            _jproc->NextEvent();
            ss << "OK";
        }else if( vals[0] == "get_object_count" ){
            //------------------ get_object_count
            ss << GetJANAObjectListJSON();
        }else if( vals[0] == "get_objects" ){  // get objects from single factory iff debug_mode is currently true
            //------------------ get_objects
            std::string factory_tag = vals.size()>=4 ? vals[3]:"";
            ss << GetJANAObjectsJSON( vals[1], vals[2], factory_tag );
        }else if( vals[0] == "fetch_objects" ){ // get objects from multiple factories regardless of debug_mode state
            //------------------ fetch_objects
            ss << FetchJANAObjectsJSON( vals );
        }else{
		    //------------------ Unknown Command
			ss << "{message:\"Bad Command: " << vals[0] << "\"}";
		}

		// Send JSON string back to requester
		zmq_send( responder, ss.str().data(), ss.str().length(), 0);
	}
}
//-------------------------------------------------------------
// GetStatusJSON
//-------------------------------------------------------------
string JControlZMQ::GetStatusJSON()
{
    // Create JSON string
    stringstream ss;
    ss << "{\n";
    ss << R"("program":"JANAcp")";  // (n.b. c++11 string literal)

	// Static info
    JSONADD( ss,"host" , _host );
    JSONADD( ss,"PID" , _pid );

    // Add numeric values to the vals map which will be converted into JSON below
    map<string,float> vals;
    
    // Get JANA status info
    JANAStatusPROC(vals);

    // Get current system info from /proc
    HostStatusPROC(vals);

    // Write all items in "vals" into the JSON formatted return string
    for( const auto &p : vals ) JSONADD(ss, p.first, p.second);

    // Close JSON string and return
    ss << "\n}";
	return ss.str(); // TODO: return this with move semantics
}

//---------------------------------
// JANAStatusPROC
//---------------------------------
void JControlZMQ::JANAStatusPROC(std::map<std::string,float> &vals)
{
	vals["NEventsProcessed"   ] = _japp->GetNEventsProcessed();
	vals["NThreads"           ] = _japp->GetNThreads();
	vals["rate_avg"           ] = _japp->GetIntegratedRate();
	vals["rate_instantaneous" ] = _japp->GetInstantaneousRate();
}

//---------------------------------
// HostStatusPROC
//---------------------------------
void JControlZMQ::HostStatusPROC(std::map<std::string,float> &vals)
{
    /// Get CPU and Memory stats for system and current process and add
    /// them to the vals dictionary. This method just defers to the
    /// system specific method.
#ifdef __linux__
    HostStatusPROCLinux( vals );
#elif defined(__APPLE__)
    HostStatusPROCMacOSX( vals );
#else
    vals["unknown_system_type"] = 1;
#endif
}

//---------------------------------
// HostStatusPROCLinux
//---------------------------------
void JControlZMQ::HostStatusPROCLinux(std::map<std::string,float> &vals)
{
#ifdef __linux__
    /// Get host info using the /proc mechanism on Linux machines.
    /// This returns the CPU usage/idle time. In order to work,
    /// it needs to take two measurements separated in time and
    /// calculate the difference. So that we don't linger here
    /// too long, we maintain static members to keep track of the
    /// previous reading and take the delta with that.

    //------------------ CPU Usage ----------------------
    static time_t last_time = 0;
    static double last_user = 0.0;
    static double last_nice = 0.0;
    static double last_sys  = 0.0;
    static double last_idle = 0.0;
    static double delta_user = 0.0;
    static double delta_nice = 0.0;
    static double delta_sys  = 0.0;
    static double delta_idle = 1.0;

    time_t now = time(nullptr);
    if(now > last_time){
        ifstream ifs("/proc/stat");
        if( ifs.is_open() ){
            string cpu;
            double user, nice, sys, idle;

            ifs >> cpu >> user >> nice >> sys >> idle;
            ifs.close();

            delta_user = user - last_user;
            delta_nice = nice - last_nice;
            delta_sys  = sys  - last_sys;
            delta_idle = idle - last_idle;
            last_user = user;
            last_nice = nice;
            last_sys  = sys;
            last_idle = idle;

            last_time = now;
        }
    }

    double norm = delta_user + delta_nice + delta_sys + delta_idle;
    double user_percent = 100.0*delta_user/norm;
    double nice_percent = 100.0*delta_nice/norm;
    double sys_percent  = 100.0*delta_sys /norm;
    double idle_percent = 100.0*delta_idle/norm;
    double cpu_usage    = 100.0 - idle_percent;

    vals["cpu_user" ] = user_percent;
    vals["cpu_nice" ] = nice_percent;
    vals["cpu_sys"  ] = sys_percent;
    vals["cpu_idle" ] = idle_percent;
    vals["cpu_total"] = cpu_usage;

    //------------------ Memory Usage ----------------------

    // Read memory from /proc/meminfo
    ifstream ifs("/proc/meminfo");
    int mem_tot_kB = 0;
    int mem_free_kB = 0;
    int mem_avail_kB = 0;
    if(ifs.is_open()){
        char buff[4096];
        bzero(buff, 4096);
        ifs.read(buff, 4095);
        ifs.close();

        string sbuff(buff);

        size_t pos = sbuff.find("MemTotal:");
        if(pos != string::npos) mem_tot_kB = strtol(&buff[pos+10+1], nullptr, 10);

        pos = sbuff.find("MemFree:");
        if(pos != string::npos) mem_free_kB = strtol(&buff[pos+9+1], nullptr, 10);

        pos = sbuff.find("MemAvailable:");
        if(pos != string::npos) mem_avail_kB = strtol(&buff[pos+14+1], nullptr, 10);
    }

    // RAM
    // reported RAM from /proc/memInfo apparently excludes some amount
    // claimed by the kernel. To get the correct amount in GB, I did a
    // linear fit to the values I "knew" were correct and the reported
    // values in kB for several machines.
    int mem_tot_GB = (int)round(0.531161471 + (double)mem_tot_kB*9.65808E-7);
    vals["ram_tot_GB"] = mem_tot_GB;
    vals["ram_free_GB"] = mem_free_kB*1.0E-6;
    vals["ram_avail_GB"] = mem_avail_kB*1.0E-6;

    // Get system resource usage
    struct rusage usage = {};
    getrusage(RUSAGE_SELF, &usage);
    double mem_usage = (double)(usage.ru_maxrss)/1024.0; // convert to MB
    vals["ram_used_this_proc_GB"] = (double)mem_usage*1.0E-3;
#endif // __linux__
}

//---------------------------------
// HostStatusPROCMacOSX
//---------------------------------
void JControlZMQ::HostStatusPROCMacOSX(std::map<std::string,float> &vals)
{
#ifdef __APPLE__

    //------------------ Memory Usage ----------------------
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    if(KERN_SUCCESS == host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count)) {
        double page_size = (double)getpagesize();
        double inactive = page_size*vmstat.inactive_count;
        double ramfree = page_size*vmstat.free_count;
        vals["ram_free_GB"] = ramfree/pow(1024.0, 3);
        vals["ram_avail_GB"] = (inactive+ramfree)/pow(1024.0, 3);
    }

    // Get total system memory (this is more stable than adding everything returned by host_statistics)
    int mib [] = { CTL_HW, HW_MEMSIZE };
    int64_t value = 0;
    size_t length = sizeof(value);
    if(sysctl(mib, 2, &value, &length, NULL, 0) != -1) vals["ram_tot_GB"] = ((double)value)/pow(1024.0, 3);

    // Get memory and CPU usage for this process
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double mem_usage = (double)(usage.ru_maxrss); // empirically, this seems to b in bytes though google claims kB (?)
    vals["ram_used_this_proc_GB"] = (double)mem_usage/pow(1024.0, 3); // convert to GB

    //------------------ CPU Usage ----------------------
    static time_t last_time = 0;
    static double last_user = 0.0;
    static double last_nice = 0.0;
    static double last_sys  = 0.0;
    static double last_idle = 0.0;
    static double delta_user = 0.0;
    static double delta_nice = 0.0;
    static double delta_sys  = 0.0;
    static double delta_idle = 1.0;

    // The following was copied from a Stack Overflow example
    processor_cpu_load_info_t cpuLoad;
    mach_msg_type_number_t processorMsgCount;
    natural_t processorCount;

    uint64_t totalSystemTime = 0, totalUserTime = 0, totalIdleTime = 0;
    kern_return_t err = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount, (processor_info_array_t *)&cpuLoad, &processorMsgCount);
    if( err == KERN_SUCCESS ) {
        for (natural_t i = 0; i < processorCount; i++) {
            // Calc load types and totals, with guards against 32-bit overflow
            // (values are natural_t)
            uint64_t system = 0, user = 0, idle = 0;

            system = cpuLoad[i].cpu_ticks[CPU_STATE_SYSTEM];
            user = cpuLoad[i].cpu_ticks[CPU_STATE_USER] + cpuLoad[i].cpu_ticks[CPU_STATE_NICE];
            idle = cpuLoad[i].cpu_ticks[CPU_STATE_IDLE];

            totalSystemTime += system;
            totalUserTime += user;
            totalIdleTime += idle;
        }
    }

    // Similar to Linux version, we must use two measurements to get a usage rate.
    time_t now = time(nullptr);
    if(now > last_time){
        double user, nice=0, sys, idle;

        user = totalUserTime;
        sys = totalSystemTime;
        idle = totalIdleTime;

        delta_user = user - last_user;
        delta_nice = nice - last_nice;
        delta_sys  = sys  - last_sys;
        delta_idle = idle - last_idle;
        last_user = user;
        last_nice = nice;
        last_sys  = sys;
        last_idle = idle;

        last_time = now;
    }

    double norm = delta_user + delta_nice + delta_sys + delta_idle;
    double user_percent = 100.0*delta_user/norm;
    double nice_percent = 100.0*delta_nice/norm;
    double sys_percent  = 100.0*delta_sys /norm;
    double idle_percent = 100.0*delta_idle/norm;
    double cpu_usage    = 100.0 - idle_percent;

    vals["cpu_user" ] = user_percent;
    vals["cpu_nice" ] = nice_percent;
    vals["cpu_sys"  ] = sys_percent;
    vals["cpu_idle" ] = idle_percent;
    vals["cpu_total"] = cpu_usage;

#else
    _DBG_<<"Calling HostStatusPROCMacOSX on non-APPLE machine. " << vals.size() << std::endl; // vals.size() is just to prevent compiler warning
#endif // __APPLE__
}

//---------------------------------
// GetDiskSpace
//---------------------------------
void JControlZMQ::GetDiskSpace(const std::string &dirname, std::map<std::string,float> &vals)
{
    // Attempt to get stats on the disk specified by dirname.
    // If found, they are added to vals. If no directory by
    // that name is found then nothing is added to vals and
    // this returns quietly.

    struct statvfs vfs = {};
    int err = statvfs(dirname.c_str(), &vfs);
    if( err != 0 ) return;

    double total_GB = vfs.f_bsize * vfs.f_blocks * 1.0E-9;
    double avail_GB = vfs.f_bsize * vfs.f_bavail * 1.0E-9;
    double used_GB  = total_GB-avail_GB;
    double used_percent = 100.0*used_GB/total_GB;

    vals[dirname+"_tot"] = total_GB;
    vals[dirname+"_avail"] = avail_GB;
    vals[dirname+"_used"] = used_GB;
    vals[dirname+"_percent_used"] = used_percent;
}

//---------------------------------
// GetJANAFactoryListJSON
//---------------------------------
std::string JControlZMQ::GetJANAFactoryListJSON()
{
    // Create JSON string
    stringstream ss;
    ss << "{\n";
    ss << R"("program":"JANAcp")";  // (n.b. c++11 string literal)

    // Static info
    JSONADD( ss,"host" , _host );
    JSONADD( ss,"PID" , _pid );
    ss << ",\n" << R"("factories":[)";

    auto component_summary = _japp->GetComponentSummary();
    bool is_first = true;
    for( auto fac_summary : component_summary.factories ){

        int indent_level = 2;
        if( !is_first ) ss << ",";
        is_first = false;
        ss << "\n" + string(indent_level*2, ' ') + "{\n";
        JSONADD( ss, "plugin_name" , fac_summary.plugin_name, indent_level, true );
        JSONADD( ss, "factory_name" , fac_summary.factory_name, indent_level );
        JSONADD( ss, "factory_tag" , fac_summary.factory_tag, indent_level );
        JSONADD( ss,"object_name" , fac_summary.object_name, indent_level );
        ss << "\n" + string(indent_level*2, ' ') + "}";
    }
    ss << "\n  ]\n";

    // Close JSON string and return
    ss << "}\n";
//    cout << ss.str() << endl;
    return ss.str(); // TODO: return this with move semantics
}

//---------------------------------
// GetJANAObjectListJSON
//---------------------------------
std::string JControlZMQ::GetJANAObjectListJSON(){
    /// Get a list of all objects in JSON format. This only reports the
    /// the factory name, tag, object type, plugin and number of objects
    /// already produced for this event.

    // Get list of factories and number of objects they've created this event already
    std::map<JFactorySummary, std::size_t> factory_object_counts;
    _jproc->GetObjectStatus( factory_object_counts );

    // Create JSON string
    stringstream ss;
    ss << "{\n";
    ss << R"("program":"JANAcp")";  // (n.b. c++11 string literal)

    // Static info
    JSONADD( ss,"host" , _host );
    JSONADD( ss,"PID" , _pid );
    JSONADD( ss,"run_number" , _jproc->GetRunNumber() );
    JSONADD( ss,"event_number" , _jproc->GetEventNumber() );
    ss << ",\n" << R"("factories":[)";

    auto component_summary = _japp->GetComponentSummary();
    bool is_first = true;
    for( auto pfac_summary : factory_object_counts ){

        auto &fac_summary = pfac_summary.first;
        auto &Nobjects    = pfac_summary.second;

        int indent_level = 2;
        if( !is_first ) ss << ",";
        is_first = false;
        ss << "\n" + string(indent_level*2, ' ') + "{\n";
        JSONADD( ss, "plugin_name" , fac_summary.plugin_name, indent_level, true );
        JSONADD( ss, "factory_name" , fac_summary.factory_name, indent_level );
        JSONADD( ss, "factory_tag" , fac_summary.factory_tag, indent_level );
        JSONADD( ss,"object_name" , fac_summary.object_name, indent_level );
        JSONADD( ss,"nobjects" , Nobjects, indent_level );
        ss << "\n" + string(indent_level*2, ' ') + "}";
    }
    ss << "\n  ]\n";

    // Close JSON string and return
    ss << "}\n";
//    cout << ss.str() << endl;
    return ss.str(); // TODO: return this with move semantics
}

//---------------------------------
// GetJANAObjectsJSON
//---------------------------------
std::string JControlZMQ::GetJANAObjectsJSON(const std::string &object_name, const std::string &factory_name, const std::string &factory_tag){
    /// Get the object contents (if possible) for the specified factory.
    /// If this is called while not in debug_mode then it will return
    /// no objects.

    // Get map of objects where key is address as hex string
    std::map<std::string, JObjectSummary> objects;
    _jproc->GetObjects( factory_name, factory_tag, object_name, objects );

    // Build map of values to create JSON record of. Add some redundant
    // info so there is the option of verifying the exact origin of this
    // by the consumer.
    std::unordered_map<std::string, std::string> mvals;
    mvals["program"] = "JANAcp";
    mvals["host"] = _host;
    mvals["PID"] = ToString(_pid);
    mvals["run_number"] = ToString(_jproc->GetRunNumber());
    mvals["event_number"] = ToString(_jproc->GetEventNumber());
    mvals["object_name"] = object_name;
    mvals["factory_name"] = factory_name;
    mvals["factory_tag"] = factory_tag;
    mvals["objects"] = JJSON_Create(objects, 2); // Create JSON of objects

    // Create JSON string
    std::string json = JJSON_Create(mvals);
//    cout << json << std::endl;
    return json;
}

//---------------------------------
// FetchJANAObjectsJSON
//---------------------------------
std::string JControlZMQ::FetchJANAObjectsJSON(std::vector<std::string> &vals){
    /// Fetch multiple objects from the next event to be processed or
    /// from the current event if debug_mode is currently true.
    /// This is intended to be used in a situation where the event processing
    /// should be allowed to continue basically uninhibited and one simply
    /// wants to spectate occasional events. E.g. a remote event monitor.
    ///
    /// Upon entry, vals will contain the full command which should look like
    ///
    ///   "fetch_objects" "factory:tag1" "factory:tag2" ...
    ///
    /// where "factory:tag" is the combined factory + tag string.

    std::set<std::string> factorytags;
    for( size_t i=1; i<vals.size(); i++ ) factorytags.insert( vals[i] );
    _jproc->SetFetchFactories( factorytags ); // this automatically sets the fetch flag

    // If we are in debug_mode then fetch the objects immediately for the 
    // current event. Note that FetchObjectsNow will clear the fetch_flag
    // so the wait loop below will exit immediately on the first iteration.
    if( _jproc->GetDebugMode() ) _jproc->FetchObjectsNow();
    
    // Build map of values to create JSON record of. Add some redundant
    // info so there is the option of verifying the exact origin of this
    // by the consumer.
    std::unordered_map<std::string, std::string> mvals;
    mvals["program"] = "JANAcp";
    mvals["host"] = _host;
    mvals["PID"] = ToString(_pid);
    
    // Wait up to 3 seconds for the fetch to finish.
    for(int i=0; i<1000; i++){
        if( !_jproc->GetFetchFlag() ) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }

    // run_number and event_number are recorded when the data is fetched
    // from the event into the metadata. Append all FetchMetadata to
    // record.
    auto metadata = _jproc->GetLastFetchMetadata();
    mvals.insert(metadata.begin(), metadata.end() );
    
     // Get the results of the fetch operation
    auto results = _jproc->GetLastFetchResult();

    // Convert all object summaries into JSON strings
    std::unordered_map<std::string, std::string> mobjvals;
    for( auto& [factorytag, objects] : results ){
        mobjvals[factorytag] = JJSON_Create(objects, 3); // Create JSON of objects
    }
    mvals["objects"] = JJSON_Create(mobjvals);

    // Create JSON string
    std::string json = JJSON_Create(mvals);
    return json;
}


