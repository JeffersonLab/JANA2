

#ifndef _MYCLUSTER_H_
#define _MYCLUSTER_H_

#include <string>

#include <JANA/JObject.h>

class MyCluster:public JObject {
	public:
	
		MyCluster(double _x_avg, double _Etot, double _t_avg):x_avg(_x_avg),Etot(_Etot),t_avg(_t_avg){}
	
		double x_avg;
		double Etot;
		double t_avg;
};

#endif   // _MYCLUSTER_H_


