/* ----------------------------------------------------------------------------
    Author: Ken C. K. Lee
    Email:  cklee@cse.psu.edu
    Web:    http://www.cse.psu.edu/~cklee
    Date:   Jan, 2008

    Copyright(c) Ken C. K. Lee 2008
    This file contains object search algorithms for graph.
---------------------------------------------------------------------------- */
#ifndef objectsearch_defined
#define objectsearch_defined

#include "collection.h"

#define MAXQUERY    10

class NodeMapping;
class ObjectSearchResult
{
public:
    const int   m_srcid;    
    const int   m_nid;
    const float m_cost;
    const float current_cost;
    Array       m_path;
    Array       m_objects;
public:
    ObjectSearchResult(const int a_nid, const float a_cost):
        m_srcid(a_nid), m_nid(a_nid), m_cost(a_cost), current_cost(a_cost)
        { m_path.append((void*)m_srcid); };
        
    ObjectSearchResult(const int a_srcid, const int a_nid, const Array& a_path, const float a_cost):
        m_srcid(a_srcid) ,m_nid(a_nid), m_path(a_path), m_cost(a_cost), current_cost(a_cost)
        { m_path.append((void*)m_nid); };
        
    ObjectSearchResult(const int a_srcid, const int a_nid,  const float a_cost, const float a_current_cost):
        m_srcid(a_srcid) ,m_nid(a_nid),  m_cost(a_cost), current_cost(a_current_cost)
        { };    
        
        
    ~ObjectSearchResult()
        { m_path.clean(); };
    void addObjects(const Array& a_objs)
    {
        for (int i=0; i<a_objs.size(); i++)
            m_objects.append(m_objects.get(i));
    }
    static int compare(const void* a0, const void* a1)
    {
        ObjectSearchResult* r0 = *(ObjectSearchResult**)a0;
        ObjectSearchResult* r1 = *(ObjectSearchResult**)a1;
        if (r0->m_cost < r1->m_cost) return -1;
        if (r0->m_cost > r1->m_cost) return +1;
        return 0;
    }
};

#endif


