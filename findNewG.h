/* ----------------------------------------------------------------------------

    Date:   Jan, 2008

    Copyright(c) Ken C. K. Lee 2008

    This file contains a class ObjectGen declaration.
    It is used to generate object on a network.
---------------------------------------------------------------------------- */
#ifndef findNewG_defined
#define findNewG_defined

#include "collection.h"
#include<vector>

class findNewG
{
public:
    static long objectSave(Array& node2object , Array& a_allnodes, int everyInterval);
    static long hierObjectSave(Array& node2object , Array& a_allnodes, vector<int>& Interval, vector<double>& radius);

};

#endif
