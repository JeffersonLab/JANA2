
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <pthread.h>

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


// The following is are convenience macros that make
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
#define JSONADD(K) ss<<"\n,\""<<K<<"\":"
#define JSONADS(K,V) ss<<"\n,\""<<K<<"\":\""<<V<<"\""

static void *JANA_ZEROMQ_CONTEXT;
static int JANA_ZEROMQ_STATS_PORT;


//-------------------------------------------------------------
// JControlZMQ
//-------------------------------------------------------------
JControlZMQ::JControlZMQ(JApplication *app, int port):_port(port),_japp(app)
{
    LOG << "Launching JControlZMQ thread ..." << LOG_END;

    // Create new zmq context, get the current host name, and launch server thread
    _zmq_context = zmq_ctx_new();
    gethostname( _host, 256 );
    _thr = new std::thread( &JControlZMQ::ServerLoop, this );

}

//-------------------------------------------------------------
// ~JControlZMQ
//-------------------------------------------------------------
JControlZMQ::~JControlZMQ(void)
{
    // Tell server thread to quit and wait for it to finish
    _done =true;
    if(_thr != nullptr) {
        LOG << "Joining JControlZMQ thread ..." << LOG_END;
        _thr->join();
        delete _thr;
        LOG << "JControlZMQ thread joined." << LOG_END;
    }

    // Delete the zmq context
    if( _zmq_context != nullptr ) zmq_ctx_destroy( _zmq_context );

}

//-------------------------------------------------------------
// ServerLoop
//-------------------------------------------------------------
void JControlZMQ::ServerLoop(void)
{
    /// This implements the zmq server for handling REQ-REP requests.
    /// It is run in a separate thread and should generally take very
    /// little CPU unless it gets pounded by requests. It will loop
    /// indefinitely until the internal _done member is set to true.
    /// Currently, that is done only by calling the destructor.

    LOG << "JControlZMQ server starting ..." << LOG_END;

    // This just makes it easier to identify this thread while debugging.
    // Linux and Mac OS X use a different calling signatures for pthread_setname_np
#if __linux__
    pthread_setname_np( pthread_self(), "JControlZMQ::ServerLoop" );
#elif defined(__APPLE__)
    pthread_setname_np( "JControlZMQ::ServerLoop" );
#endif

    // Bind to port number specified in constructor. Most likely this came from JANA_ZMQ_PORT config. parameter
    char bind_str[256];
	sprintf( bind_str, "tcp://*:%d", _port );
	void *responder = zmq_socket( _zmq_context, ZMQ_REP );
	int rc = zmq_bind( responder, bind_str);
	if( rc != 0 ){
		LOG << "JControlZMQ: Unable to bind zeroMQ control socket " << _port << "!" << LOG_END;
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
				LOG << "JControlZMQ: ERROR listening on control socket: errno=" << errno << LOG_END;
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
		    if( vals.size()>1 )  _japp->SetExitCode( atoi(vals[1].c_str()) ); // allow remote user to optionally set exit code.
            _japp->Quit();
			ss << "{message:\"OK\"}";;
		}else if( vals[0] == "get_status" ){
		    //------------------ get_status
			ss << GetStatusJSON();
		}else if( vals[0] == "get_file_size" ){ // mulitple files may be specified
		    //------------------ get_file_size
			if( vals.size()<2){
				ss << "{message:\"ERROR: No file given!\"}";
			}else{
			    ss << "{";
				for( uint32_t i=1; i<vals.size(); i++){

					auto &fname = vals[i];
					struct stat st;
					int64_t fsize = -1;
					if( stat(fname.c_str(), &st) == 0) fsize = (int64_t)st.st_size;
					if( i>1 ) ss << ",";
					ss << "\"" << fname << "\"" << ":" << fsize << "\n";
				}
				ss << "}";
			}
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
string JControlZMQ::GetStatusJSON(void)
{
    // Create JSON string
    stringstream ss;
    ss << "{\n";
    ss << "\"program\":\"JANAcp\"";

    JSONADS( "host" , _host );

    // Add numeric values to the vals map which will be converted into JSON below
    map<string,float> vals;

    // Get current system info from /proc
    HostStatusPROC(vals);

    // Write all items in "vals" into the JSON formatted return string
    for( auto p : vals ) JSONADD(p.first) << p.second;

    // Close JSON string and return
    ss << "\n}";
	return ss.str(); // TODO: return this with move semantics
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

    time_t now = time(NULL);
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
        if(pos != string::npos) mem_tot_kB = atoi(&buff[pos+10+1]);

        pos = sbuff.find("MemFree:");
        if(pos != string::npos) mem_free_kB = atoi(&buff[pos+9+1]);

        pos = sbuff.find("MemAvailable:");
        if(pos != string::npos) mem_avail_kB = atoi(&buff[pos+14+1]);
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
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double mem_usage = (double)(usage.ru_maxrss)/1024.0; // convert to MB
    vals["ram_used_this_proc_GB"] = (double)mem_usage*1.0E-9;
#endif // __linux__
}

//---------------------------------
// HostStatusPROCMacOSX
//---------------------------------
void JControlZMQ::HostStatusPROCMacOSX(std::map<std::string,float> &vals)
{
#ifdef __APPLE__
    // TODO: Fill this in with code to read from Darwin systems. Check this out:
    //  https://gist.github.com/gunkmogo/5d54f9fb4579768d9c7d5c41293cc784

    // Get system resource usage
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double mem_usage = (double)(usage.ru_maxrss)/1024.0; // convert to MB
    vals["ram_used_this_proc_GB"] = (double)mem_usage*1.0E-9;
#endif // __APPLE__
}

//---------------------------------
// GetDiskSpace
//---------------------------------
void JControlZMQ::GetDiskSpace(std::string dirname, std::map<std::string,float> &vals)
{
    // Attempt to get stats on the disk specified by dirname.
    // If found, they are added to vals. If no directory by
    // that name is found then nothing is added to vals and
    // this returns quietly.

    struct statvfs vfs;
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


