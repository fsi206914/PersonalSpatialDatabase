/* ----------------------------------------------------------------------------
    Author: Ken C. K. Lee
    Email:  cklee@cse.psu.edu
    Web:    http://www.cse.psu.edu/~cklee
    Date:   Jan, 2008

    Copyright(c) Ken C. K. Lee 2008

    This file contains a class ObjectGen declaration.
    It is used to generate object on a network.
---------------------------------------------------------------------------- */
#ifndef AstarSearch_defined
#define AstarSearch_defined

#include "collection.h"
using namespace std;
class NodeMapping;



class AstarSearch
{
public:
    static void Astar(  NodeMapping& a_map, int PQ_objectID, Array& a_allnodes, int src_ID);

};
#endif
