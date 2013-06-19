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
#ifndef baseWatchtower_defined
#define baseWatchtower_defined

#include "collection.h"
#include <vector>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <iostream>
using namespace std;




class watchtowerObject
{
    friend std::ostream & operator<<(std::ostream &os, const watchtowerObject &wo);
    friend class boost::serialization::access;
public:
    int   Object_ID;     // neighbor node id
    float     cost;    // cost between the object and the watchtower
    float     edgeCost;    // cost of the edge towards neighbor
    int     watchtower_level;     // neighbor node id

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & Object_ID & cost & edgeCost & watchtower_level;
    }
public:
    // constructor/destructor
    watchtowerObject(int m_id, float a_interval, float a_edgeCost, int level=0):
       Object_ID(m_id), cost(a_interval), edgeCost(a_edgeCost), watchtower_level(level) {};
    watchtowerObject(){};

    virtual ~watchtowerObject() {};

    static int compare(const void* a0, const void* a1)
    {
        watchtowerObject* r0 = *(watchtowerObject**)a0;
        watchtowerObject* r1 = *(watchtowerObject**)a1;
        if (r0->cost < r1->cost) return -1;
        if (r0->cost > r1->cost) return +1;
        return 0;
    }

};



inline std::ostream & operator<<(std::ostream &os, const watchtowerObject &wo)
{
    return os << " " << wo.Object_ID << "  " << wo.cost << "  " <<wo.edgeCost << "  " <<wo.watchtower_level;
}


class watchtowerArray
{
    friend std::ostream & operator<<(std::ostream &os, const watchtowerArray &wo);
    friend class boost::serialization::access;
public:
    typedef watchtowerObject * watchtowerArray_Pointer;
    std::vector<watchtowerArray_Pointer> schedule;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & schedule;
    }
public:
    // constructor/destructor

    watchtowerArray(){};
    void append(watchtowerArray_Pointer wo)
    {
        schedule.push_back(wo);
    }
    virtual ~watchtowerArray() {};
};

inline std::ostream & operator<<(std::ostream &os, const watchtowerArray &wo)
{
    std::vector<watchtowerObject *> ::const_iterator it;
    for(it = wo.schedule.begin(); it != wo.schedule.end(); it++){
        os << *(*it)<<"  ";
    }
    return os;
}




class baseWatchtower
{
public:
    int     m_watchtowerID;     // neighbor node id
    float   m_src_cost;         // cost of the edge towards neighbor
    int     srcNode_ID;
    Array   m_wtObject;
public:
    // constructor/destructor
    baseWatchtower(const int m_id, const float a_cost, const int a_ID=0):
       m_watchtowerID(m_id), m_src_cost(a_cost), srcNode_ID(a_ID) {};
    virtual ~baseWatchtower() {};

    void addwtObject(watchtowerObject& a_watchtowerObject)
    {
    m_wtObject.append(new watchtowerObject(a_watchtowerObject.Object_ID, a_watchtowerObject.cost, a_watchtowerObject.watchtower_level));
    };

    // storage size
//    int size() const
//    {
//        return sizeof(m_neighbor)+sizeof(m_cost);
//    };

};


class nodeWatchtower
{
public:
    const int     m_src_ID;     // neighbor node id
    int     interval;    // it is to represent the level of watchtowers

public:
    // constructor/destructor
    nodeWatchtower(const int m_id, int a_interval=0):
       m_src_ID(m_id), interval(a_interval) {};
    virtual ~nodeWatchtower() {};
    //
    // storage size
//    int size() const
//    {
//        return sizeof(m_neighbor)+sizeof(m_cost);
//    };

};


class HierarchyInfo
{
public:
    const int     Max_layer;     // neighbor node id
    vector<float>   level_R;    // its to represent the level of watchtowers
    vector<int> level_interval;
public:
    // constructor/destructor
    HierarchyInfo(const int a_Max_layer):
       Max_layer(a_Max_layer){};
    virtual ~HierarchyInfo() {};
    //
    // storage size
//    int size() const
//    {
//        return sizeof(m_neighbor)+sizeof(m_cost);
//    };

};

#endif
