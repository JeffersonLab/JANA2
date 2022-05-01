
// This stuff is needed by rootcint to generate the dictionary files
#if defined(__CINT__) || defined(__CLING__)

namespace jana{}
using namespace jana;

class pthread_t;
class pthread_cond_t;
class pthread_mutex_t;

class exception;
class __signed;
class timespec;
class timeval;

#endif // __CINT__  __CLING__


// This stuff is needed when actually compiling the dictionary files with g++
#ifdef G__DICTIONARY

namespace jana{}
using namespace jana;

#ifndef _root_cint_seen_
#define _root_cint_seen_

#endif // _root_cint_seen_

#endif
