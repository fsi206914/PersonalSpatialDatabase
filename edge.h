/* ----------------------------------------------------------------------------
    Author: Ken C. K. Lee
    Email:  cklee@cse.psu.edu
    Web:    http://www.cse.psu.edu/~cklee
    Date:   Jan, 2008

    Copyright(c) Ken C. K. Lee 2008

    This file contains a class Edge declaration.
    This presents an edge that
    * points a neighboring node and
    * carries a cost to that node.
---------------------------------------------------------------------------- */
#ifndef edge_defined
#define edge_defined

#include "baseWatchtower.h"
#include <iostream>

using namespace std;

class baseWatchtower;

class Edge
{
public:
    int     m_neighbor;     // neighbor node id
    float   m_cost;         // cost of the edge towards neighbor
    long    m_ID;     // edge id
    int     m_srcID;
    Array   m_watchtowers;    // a set of watchtowers in this edge
    Array   selected_watchtowers;    // a set of selected watchtowers
    watchtowerArray   *WA;
    bool personal;

public:
    // constructor/destructor
    Edge(const int a_neighbor, const float a_cost, const long a_id = 0, watchtowerArray *a_WA = 0, int a_m_srcID = 0):
      m_neighbor(a_neighbor), m_cost(a_cost),  m_ID(a_id)
      {
        WA = a_WA; personal = false; m_srcID = a_m_srcID;
      };
    virtual ~Edge() {};
    //
    void setSerial(watchtowerArray* a_WA)
    {
        WA = a_WA;
    };

    void setPersonal( bool a_true)
    {
        personal = a_true;
    };

    bool getPersonal()
    {
        return personal;
    };


    void addWatchtowers(baseWatchtower& a_baseWatchtower)
    {
        m_watchtowers.append( new baseWatchtower(a_baseWatchtower.m_watchtowerID, a_baseWatchtower.m_src_cost) );
    };

    void addSelected(int a, float b, float d, int c)
    {
        watchtowerObject *select = new watchtowerObject(a,b,d,c);
        selected_watchtowers.append(select);
        if(WA != NULL)
        {
            WA->append(select);
        }
    };

    int getWTsize()
    {
        return m_watchtowers.size();
    }
    // storage size
    int size() const
    {
        return sizeof(m_neighbor)+sizeof(m_cost);
    };
};

#endif

