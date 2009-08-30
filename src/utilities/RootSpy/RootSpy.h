

#ifndef _ROOTSPY_H_
#define _ROOTSPY_H_

#include <iostream>
#include <iomanip>
using namespace std;


class rs_mainframe;
class rs_cmsg;
class rs_info;

extern rs_mainframe *RSMF;
extern rs_cmsg *RS_CMSG;
extern rs_info *RS_INFO;

#ifndef _DBG_
#define _DBG_  cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ cerr<<__FILE__<<":"<<__LINE__<<endl
#endif



#endif //_ROOTSPY_H_
