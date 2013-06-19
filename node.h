/* ----------------------------------------------------------------------------
    Author: Ken C. K. Lee
    Email:  cklee@cse.psu.edu
    Web:    http://www.cse.psu.edu/~cklee
    Date:   Jan, 2008

    Copyright(c) Ken C. K. Lee 2008

    This file contains a class Node declaration.
    It consists of node information and edges to neighbors.
---------------------------------------------------------------------------- */
#ifndef node_defined
#define node_defined

#include "collection.h"
#include "baseWatchtower.h"

class Edge;
class baseWatchtower;

class Node
{
public:
    int     m_id;       // node id, that should be unique
    float   m_x, m_y;   // the coordinate of the node
    Array   m_edges;    // a set of edges to neighbors
    Array   m_watchtowers;    // a set of edges to neighbors
    Array   selected_watchtowers;    // a set of edges to neighbors
    int remainInterval;
    watchtowerArray   *WA;
    bool personal;

public:
    // constructor/destructor
    Node(const int a_id, const float a_x=0, const float a_y=0, int a_interval=0, watchtowerArray *a_WA = 0);
    virtual ~Node();
    //
    // search
    float cost(const int a_node);
    //
    // edge manipulations
    void addEdge(Edge& a_edge);

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
        m_watchtowers.append(new baseWatchtower(a_baseWatchtower.m_watchtowerID,a_baseWatchtower.m_src_cost));
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

    void delEdge(Edge& a_edge);
    //
    // memory operations
    virtual void toMem(char* a_mem, int& a_len) const;  // write to mem
    virtual void fromMem(char* a_mem, int& a_len);      // load from mem
    //
    static int compareid(const void* a0, const void* a1);
};

#endif
