
#include <string>

#ifndef _MyHit_h_
#define _MyHit_h_

class MyHit:public JObject{
	public:

		MyHit(double _x, double _E, double _t):x(_x),E(_E),t(_t){}

		double x;
		double E;
		double t;
};

#endif // _MyHit_h_

