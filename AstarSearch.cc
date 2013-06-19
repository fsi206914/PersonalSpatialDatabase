#include "collection.h"
#include "node.h"
#include "edge.h"
#include "objectgen.h"
#include "AstarSearch.h"
#include "objectsearch.h"
#include "baseWatchtower.h"
#include "nodemap.h"
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

typedef  struct QualifiedWatchtower
{
    int object_ID;
    float current_cost;
    int level;
    float farFromObject;

    bool   operator <  (const   QualifiedWatchtower&   rhs   )  const   //in asending order
    {
         return   current_cost   <   rhs.current_cost;
    }

    bool   operator > (const   QualifiedWatchtower&   rhs   )  const   //in descending order
    {
         return   current_cost   >  rhs.current_cost;
    }

}QualifiedWatchtower;

Edge * findEdgesObjectSearch(int src_node, int dest_node, Array& a_allnodes)
{

     Node* node = (Node*)a_allnodes.get(src_node);
//     cerr << "node_ID:    "<<node->m_id<<endl;
     Edge *alternative;
     for(int j=0;j < node->m_edges.size(); j++)
     {
         alternative = (Edge*) node->m_edges[j];
         if(alternative->m_neighbor==dest_node)
         {
             return alternative;
         }
     }
     return 0;
}


void lookForOneWatchtower(Edge *edge, int target, float current_cost, vector<QualifiedWatchtower>& QW, bool flag=0)
{
     for(int i=edge->m_watchtowers.size()-1; i>=0; i--)
     {
         baseWatchtower *bw = (baseWatchtower*) edge->m_watchtowers[i];
         if(bw->m_wtObject.size()>0)
         {
             for(int j=bw->m_wtObject.size()-1; j>=0; j--)
             {
                 watchtowerObject *wo = (watchtowerObject*) bw->m_wtObject[j];
                 QualifiedWatchtower temp;

                 if(wo->Object_ID == target)
                 if(flag==0)
                 {temp.object_ID = wo->Object_ID; temp.current_cost= current_cost+bw->m_src_cost; temp.level = wo->watchtower_level ;
//                  cerr<<"---flag==0: "<<temp.current_cost<<"    "<<temp.object_ID<<"    "<<temp.level<<endl;
                  temp.farFromObject = wo->cost;
                  QW.push_back(temp);
                 }

                 if(wo->Object_ID == target)
                 if(flag==true)
                 { temp.object_ID = wo->Object_ID; temp.current_cost =  edge->m_cost - bw->m_src_cost + current_cost; temp.level = wo->watchtower_level;
//                  cerr<<"----flag==1: "<<temp.current_cost<<"    "<<temp.object_ID<<"    "<<temp.level<<endl;
                   temp.farFromObject = wo->cost;
                   QW.push_back(temp);
                 }
             }
         }
     }
}


float searchWatchtower(int srcID, Array &a_allnodes, int target, float &dist)
{
    Set visited(1000);
    Set edge_visited(1000);
    BinHeap h(ObjectSearchResult::compare);
    h.insert(new ObjectSearchResult(srcID,0));
    vector<QualifiedWatchtower> QW;
    while (!h.isEmpty())
    {
        ObjectSearchResult* c = (ObjectSearchResult*)h.removeTop();
        //---------------------------------------------------------------------
        // Check if the node is visited. If so, skip it.
        //---------------------------------------------------------------------

        if (visited.in((void*)c->m_nid))
        {
            delete c;
            continue;
        }
        visited.insert((void*)c->m_nid);
        Node* node = (Node*)a_allnodes.get(c->m_nid);
        for (int e=0; e<node->m_edges.size(); e++)
        {
            Edge* edge = (Edge*)node->m_edges.get(e);
            h.insert(
                new ObjectSearchResult(c->m_nid, edge->m_neighbor, c->m_path,
                c->m_cost + edge->m_cost));
        }

        int srcid = c->m_nid;

        for (int e=0; e<node->m_edges.size(); e++)
        {
            Edge* edge = (Edge*)node->m_edges.get(e);
            if(!edge_visited.in((void*)edge->m_ID))
            {
                lookForOneWatchtower(edge, target, c->m_cost, QW);
                edge_visited.insert((void*)edge->m_ID);
            }
//            cerr<<"edge ID=="<<edge->m_ID<<endl;
            int detid = edge->m_neighbor;
//            cerr<<"detid=="<<detid<<endl;
            Edge* edge_alternative = findEdgesObjectSearch(detid, srcid, a_allnodes);
//            cerr<<"edge_alternative ID=="<<edge_alternative->m_ID<<endl;
            if(!edge_visited.in((void*)edge_alternative->m_ID))
            {
                lookForOneWatchtower(edge_alternative, target, c->m_cost, QW, true);
                edge_visited.insert((void*)edge_alternative->m_ID);
            }
        }

        sort(QW.begin(), QW.end(),less<QualifiedWatchtower>());   //或者sort(ctn.begin(), ctn.end())  默认情况为升序
        if(QW.size()>0)
            break;
    }

    while (!h.isEmpty())
        delete (ObjectSearchResult*)h.removeTop();
    h.clean();

    dist = QW[0].farFromObject;
//    cerr<<"farFromObject=="<<QW[0].farFromObject<<endl;
    return QW[0].current_cost;

}



void AstarSearch::Astar(NodeMapping& a_map, int PQ_objectID, Array& a_allnodes, int src_ID)
{

         int target = PQ_objectID;
         float dist, df, de;// corresponding distance saved, expanded distance, current distance from source node.

//-------------------------begin  expanding----------------------
         Set visited(1000);
         BinHeap h(ObjectSearchResult::compare);
         h.insert(new ObjectSearchResult(src_ID,0));
         while (!h.isEmpty())
         {
         ObjectSearchResult* c = (ObjectSearchResult*)h.removeTop();
//       cout<<"ObjectSearchResult_srcnode: "<<c->m_srcid<<"destnode: "<<c->m_nid<<endl;
        //---------------------------------------------------------------------
        // Check if the node is visited. If so, skip it.
        //---------------------------------------------------------------------
        if (visited.in((void*)c->m_nid))
        {
            delete c;
            continue;
        }
        visited.insert((void*)c->m_nid);
        Node* node = (Node*)a_allnodes.get(c->m_nid);

        for (int e=0; e<node->m_edges.size(); e++)
        {
            Edge* edge = (Edge*)node->m_edges.get(e);
            df = searchWatchtower(edge->m_neighbor, a_allnodes, target, dist);
            de = c->current_cost + edge->m_cost;
            h.insert(
                new ObjectSearchResult(c->m_nid, edge->m_neighbor,
                de + dist - df, c->current_cost + edge->m_cost ));
        }

        const Array* objs = a_map.findObject(c->m_nid);
        if (objs != 0)
        {
            void * temp = (void *) objs->get(0);
            if(temp == (void*)target)
            {cerr<<"objs=="<<temp<<endl;  break;}
        }
    }
    cerr<<"============================================="<<endl;
        while (!h.isEmpty())
            delete (ObjectSearchResult*)h.removeTop();
        h.clean();
}
