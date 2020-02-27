
#ifndef _JControlZMQ_h_
#define _JControlZMQ_h_

#include <thread>
#include <JANA/JApplication.h>
#include <zmq.h>

class JControlZMQ  {
public:

    JControlZMQ(JApplication *app, int port);
    ~JControlZMQ(void);

           void ServerLoop(void);
    std::string GetStatusJSON(void);
           void HostStatusPROC(std::map<std::string,float> &vals);
           void HostStatusPROCLinux(std::map<std::string,float> &vals);
           void HostStatusPROCMacOSX(std::map<std::string,float> &vals);
           void GetDiskSpace(std::string dirname, std::map<std::string,float> &vals);

private:

    bool         _done         = false;
    JApplication *_japp        = nullptr;
    int           _port        = 0;
    void         *_zmq_context = nullptr;
    std::thread  *_thr         = nullptr;
    char         _host[256]    = "";
};


#endif // _JControlZMQ_h_

