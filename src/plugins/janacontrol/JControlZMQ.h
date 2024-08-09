
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JControlZMQ_h_
#define _JControlZMQ_h_

#include <thread>
#include <JANA/JApplication.h>
#include <zmq.h>
#include <unistd.h>

#include "JControlEventProcessor.h"

class JControlZMQ  {
public:

    JControlZMQ(JApplication *app, int port);
    ~JControlZMQ();

           void ServerLoop();
    std::string GetStatusJSON();
           void JANAStatusPROC(std::map<std::string,float> &vals);
    static void HostStatusPROC(std::map<std::string,float> &vals);
    static void HostStatusPROCLinux(std::map<std::string,float> &vals);
    static void HostStatusPROCMacOSX(std::map<std::string,float> &vals);
    static void GetDiskSpace(const std::string &dirname, std::map<std::string,float> &vals);
    std::string GetJANAFactoryListJSON();
    std::string GetJANAObjectListJSON();
    std::string GetJANAObjectsJSON(const std::string &object_name, const std::string &factory_name, const std::string &factory_tag);
    std::string FetchJANAObjectsJSON(std::vector<std::string> &vals);

private:

    bool         _done         = false;
    JApplication *_japp        = nullptr;
    int           _port        = 0;
    void         *_zmq_context = nullptr;
    std::thread  *_thr         = nullptr;
    char         _host[256]    = {};
    pid_t        _pid          = 0;
    JControlEventProcessor *_jproc = nullptr;
};


#endif // _JControlZMQ_h_

