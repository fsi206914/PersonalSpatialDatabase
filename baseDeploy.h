/* ----------------------------------------------------------------------------
    Author: Ken C. K. Lee
    Email:  cklee@cse.psu.edu
    Web:    http://www.cse.psu.edu/~cklee
    Date:   Jan, 2008

    Copyright(c) Ken C. K. Lee 2008

    This file contains a class ObjectGen declaration.
    It is used to generate object on a network.
---------------------------------------------------------------------------- */
#ifndef baseDeploy_defined
#define baseDeploy_defined

#include "collection.h"
#include <vector>

using namespace std;

class Node;

class baseDeploy
{

public:
    static int deploy(Array& a_allnodes, Array& a_alledges, float a_lambda);
    static void updateWatchtower(Array& a_allnodes, Array& a_alledges);
    static int findHotzoneWT(Array& a_allnodes, Array& a_alledges);

};

#endif
