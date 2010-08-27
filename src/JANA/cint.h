
// This stuff is needed by rootcint to generate the dictionary files
#ifdef __CINT__

namespace jana{}
using namespace jana;

class pthread_t;
class pthread_cond_t;
class pthread_mutex_t;

class exception;
class __signed;
class timespec;
class timeval;

#endif // __CINT__


// This stuff is needed when actually compiling the dictionary files with g++
#ifdef G__DICTIONARY

namespace jana{}
using namespace jana;

#ifndef _root_cint_seen_
#define _root_cint_seen_

#endif // _root_cint_seen_

#endif
