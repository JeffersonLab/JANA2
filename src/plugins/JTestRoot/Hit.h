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

    // These members are here as examples. There are no limitations on what types
    // of objects can be members here other than what the ROOT TObject may impose.
    char my_char_t                   = 'C';       // will display as signed integer
    unsigned char my_unsigned_char_t = 128;       // will display as hex number
    char my_char_array[256]          ="test";
    unsigned long my_unsigned_long_t = 12345678;
    long my_long_t                   = -12345678;
    uint32_t my_uint32_t             = 32;
    uint64_t my_uint64_t             = 64;
    std::string my_string_t          = "dave";
    int32_t my_int32_t               = -32;
    int64_t my_int64_t               = -64;

    ClassDef(Hit,1) // n.b. make sure ROOT_GENERATE_DICTIONARY is in cmake file!
};

#endif // _HIT_H_
