// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _HIT_H_
#define _HIT_H_

#include <TObject.h>

class Hit:public TObject{

public:

    Hit(int _iw, int _iv, double _E, float _t):iw(_iw),iv(_iv),E(_E),t(_t){}

    int iw;
    int iv;
    double E;
    float t;

    char my_char_t;
    unsigned char my_unsigned_char_t;
    char my_char_array[256];
    unsigned long my_unsigned_long_t;
    long my_long_t;
    uint32_t my_uint32_t;
    uint64_t my_uint64_t;
    std::string my_string_t = "dave";
    int32_t my_int32_t;
    int64_t my_int64_t;

    ClassDef(Hit,1) // n.b. make sure ROOT_GENERATE_DICTIONARY is in cmake file!
};

#endif // _HIT_H_
