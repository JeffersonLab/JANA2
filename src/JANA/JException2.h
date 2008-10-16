// JException2.h

// Base exception class for JANA package

// EJW, JLab, 19-apr-2007



#ifndef _JException2_h_
#define _JException2_h_


#include <exception>
#include <string>
#include <sstream>
#include <string.h>
#include <malloc.h>


// for stack trace
#ifdef SunOS
#else
#include <execinfo.h>
#include <cxxabi.h>
#endif



//  not sure if this is needed...also, what about __FUNCTION__
/** 
 * How to use SOURCEINFO macro in JException2 constructor:
 *
 *  throw(JException2("This is a devilish error", 666, SOURCEINFO));
*/
#define SOURCEINFO  __FILE__,__LINE__         



/** JANA packages lives in the jana namespace. */
namespace jana {

using namespace std;



/**
 * JANA exception class contains text, code, source file info, and trace info.
 * Note that trace does not work on all architectures.
 *
 * Trace is automatically generated.  On linux must link with -rdynamic to get useful trace.
 */
class JException2 : public exception {

public:
  JException2(const string &txt);
  JException2(const string &txt, const char *file, int line);
  JException2(const string &txt, int c);
  JException2(const string &txt, int c, const char *file, int line);
  virtual ~JException2(void) throw() {}  /**<Destructor does nothing.*/

  virtual string toString(void) const throw();
  virtual string toString(bool includeTrace) const throw();
  virtual const char* what(void) const throw();

  static string getStackTrace(void);


public:
  string text;     /**<Exception text.*/
  int code;        /**<Exception code.*/
  string source;   /**<Exception source file info.*/
  string trace;    /**<Stack trace generated automatically.  Does not work on all architectures.*/
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


/** 
 * Constructor given text field.
 *
 * @param txt Exception text
 */
JException2::JException2(const string &txt) : text(txt), code(0), source(""), trace(getStackTrace()) {}


//-----------------------------------------------------------------------------


/** 
 * Constructor given text field and source file info.
 *
 * @param txt Exception text
 * @param file Name of file where exception raised
 * @param line Line in file where exception raised
 */
JException2::JException2(const string &txt, const char *file, int line) : text(txt), code(0), trace(getStackTrace()) {
  stringstream ss;
  ss << "File: " << file << ",   Line: " << line << ends;
  source=ss.str();
}


//-----------------------------------------------------------------------------


/** 
 * Constructor given text field and code.
 *
 * @param txt Exception text
 * @param c   Exception code
 */
JException2::JException2(const string &txt, int c) : text(txt), code(c), source(""), trace(getStackTrace()) {}


//-----------------------------------------------------------------------------


/** 
 * Constructor given text field, code, and source file info.
 *
 * @param txt Exception text
 * @param c   Exception code
 * @param file Name of file where exception raised
 * @param line Line in file where exception raised
 */
JException2::JException2(const string &txt, int c, const char *file, int line) : text(txt), code(c), trace(getStackTrace()) {
  stringstream ss;
  ss << "File: " << file << ",   Line: " << line << ends;
  source=ss.str();
}


//-----------------------------------------------------------------------------


/** 
 * Returns string representation of exception including trace.
 *
 * @return String representation of exception
 */
string JException2::toString(void) const throw() {
  return(toString(true));
}


//-----------------------------------------------------------------------------


/** 
 * Returns string representation of exception.
 *
 * @param includeTrace true to include trace info
 *
 * @return String representation of exception
 */
string JException2::toString(bool includeTrace) const throw() {
  ostringstream oss;
  oss << endl << "?JException2:    code = " << code << "    text = " << text << endl << endl;
  if(source.size()>0) oss << source << endl << endl;
  if(includeTrace&&(trace.size()>0))  oss << "Stack trace:" << endl << endl << trace << endl;
  oss << ends;
  return(oss.str());
}


//-----------------------------------------------------------------------------


/** 
 * Returns char* representation of exception.
 *
 * @return char* representation of exception
 */
const char *JException2::what(void) const throw() {
  return(toString().c_str());
}


//-----------------------------------------------------------------------------


/** 
 * Generates stack trace using Linux system calls, doesn't work on solaris.
 *
 * @return String containing stack trace
 */
string JException2::getStackTrace(void) {

#ifdef SunOS
  return("");

#else
  size_t dlen;
  char dname[1024];
  void *trace[1024];
  int status;
  
  
  // get trace messages
  int trace_size = backtrace(trace,1024);
  if(trace_size>1024)trace_size=1024;
  char **messages = backtrace_symbols(trace, trace_size);
  
  // de-mangle and create string
  stringstream ss;
  for(int i=0; i<trace_size; ++i) {
    
    // find first '(' and '+'
    char *ppar  = strchr(messages[i],'(');
    char *pplus = strchr(messages[i],'+');
    if((ppar!=NULL)&&(pplus!=NULL)) {
      
      // replace '+' with nul, then get de-mangled name
      *pplus='\0';
      abi::__cxa_demangle(ppar+1,dname,&dlen,&status);
      
      // add to stringstream
      *(ppar+1)='\0';
      *pplus='+';
      ss << "   " << messages[i] << dname << pplus << endl;
      
    } else {
      ss << "   " << messages[i] << endl;
    }
    
  }
  ss << ends;
  
  free(messages);
  return(ss.str());
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


} // namespace jana

#endif /* _JException2_h_ */
