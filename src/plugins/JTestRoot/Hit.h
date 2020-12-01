// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <TObject.h>

class Hit:public TObject{

public:

    Hit(int _iw, int _iv, double _E):iw(_iw),iv(_iv),E(_E){}

    int iw;
    int iv;
    double E;
};
