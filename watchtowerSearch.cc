#include "node.h"
#include "edge.h"
#include "objectgen.h"
#include "baseWatchtower.h"
#include "objectsearch.h"
#include "watchtowerSearch.h"
#include "AstarSearch.h"
#include "nodemap.h"
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <stdint.h>
#include <glog/logging.h>
#include <algorithm>
#include "searchQueue.h"
using namespace std;

const bool debug = false;
int watchtowerAdd = 0;
int watchtowerMet = 0;
int atLeastOneObjWT = 0;

struct QualifiedWatchtower;
class NodeObject;
class cluster
{
    public:
        int clust_ID;
        float x;
        float y;
        float var;
        double r;

    cluster(int a_clust_ID, float a_x, float a_y, float a_var, double a_r)
    {
        x = a_x; y = a_y; var = a_var; clust_ID = a_clust_ID; r = a_r;
    };
    cluster(){};
    ~cluster(){};
};

Edge * findEdgesinObjectSearch(int src_node, int dest_node, Array& a_allnodes)
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


void lookForWatchtower(Edge *edge, searchQueue &SQ, float current_cost, const int KNN = 9999, bool flag=0)
{
     watchtowerMet++;
     bool first = true;
     for(int i=0; i< edge->selected_watchtowers.size(); i++)
     {
         if(first == true) {
             atLeastOneObjWT ++;
             first = false;
         }
         if(i>KNN)
         {
//             cout<<"KNN ="<<KNN<<endl;
             break;
         }
         watchtowerAdd++;

         watchtowerObject *wo = (watchtowerObject*) edge->selected_watchtowers[i];

         QualifyPointer temp;
         if(flag==0)
         {
            temp = new QualifiedWatchtower(wo->Object_ID, current_cost+wo->edgeCost, wo->watchtower_level, wo->cost);
         }
         if(flag==true)
         {
            temp = new QualifiedWatchtower(wo->Object_ID, edge->m_cost - wo->edgeCost + current_cost, wo->watchtower_level, wo->cost);
         }
         SQ.addWatchtower(temp);
     }
}



void lookForWatchtower(Node *node, searchQueue &SQ, float current_cost, const int KNN = 9999)
{
     watchtowerMet++;
     bool first = true;
     for(int i=0; i< node->selected_watchtowers.size(); i++)
     {
         if(first == true) {
             atLeastOneObjWT ++;
             first = false;
         }

         if(i>KNN) break;

         watchtowerAdd++;

         watchtowerObject *wo = (watchtowerObject*) node->selected_watchtowers[i];
         QualifyPointer temp = new QualifiedWatchtower(wo->Object_ID, current_cost, wo->watchtower_level, wo->cost);
         SQ.addWatchtower(temp);
     }

}

// --------------------------------------------------------------------
// check if object is found
// --------------------------------------------------------------------
int checkObject(NodeMapping& a_map, searchQueue &SQ, ObjectSearchResult* c, const int layer = 1)
{

    Array* objs = a_map.findObject(c->m_nid);
    QualifyPointer temp;
    if (objs != 0)
    {
        for(int i=0; i<objs->size(); i++)
        {
            void * no = (void *) objs->get(i);
            int ObjectID =  ( intptr_t )no;
//            cout<<"Object "<<ObjectID<<" found in the query SSSP process"<<endl;
            temp = new QualifiedWatchtower(ObjectID, c->m_cost, layer, 0);
            SQ.addWatchtower(temp);
        }
        return objs->size();
    }
    return 0;
};


bool JudgeStop(searchQueue &SQ, int k, int current_level)
{
     SQ.setLayer(current_level+1);
     int candidates_num = SQ.checkBound();
//     cout<<"candidates_num = "<<candidates_num<<endl;
     if(SQ.checkBound() >=k)
        return true;
     else return false;
}

void watchtowerSearch::oneLayerSearch(Array& node2query, Array& allnodes, NodeMapping& map, float expansion)
{
    //-------------------------------------------------------------------------
    // initialization
    //-------------------------------------------------------------------------
    for(int query_i = 0; query_i<node2query.size(); query_i++)
    {
        NodeObject *nq = (NodeObject*) node2query.get(query_i);
        int nodeid = nq->m_nodeid;

        int a_src = nodeid;
        bool stop=false;
        Set visited(1000);

        Set edge_visited(1000);

        vector<QualifiedWatchtower> PQ;
        BinHeap h(ObjectSearchResult::compare);
        h.insert(new ObjectSearchResult(a_src,0));
        searchQueue SQ(true);

        //---------we may reduce KNN number when we found objects in the query SSSP expansion.
        int updateKNN = KNN;
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
            Node* node = (Node*)allnodes.get(c->m_nid);
            for (int e=0; e<node->m_edges.size(); e++)
            {
                Edge* edge = (Edge*)node->m_edges.get(e);
                h.insert(
                    new ObjectSearchResult(c->m_nid, edge->m_neighbor, c->m_path,
                    c->m_cost + edge->m_cost));
            }
            // --------------------------------------------------------------------
            // check if watchtower is found
            // --------------------------------------------------------------------

            int srcid = c->m_nid;
            // --------------------------------------------------------------------
            // first it search the current node, then search every edge
            // --------------------------------------------------------------------

            lookForWatchtower(node, SQ, c->m_cost, updateKNN);
            for (int e=0; e<node->m_edges.size(); e++)
            {
                Edge* edge = (Edge*)node->m_edges.get(e);
                if(!edge_visited.in((void*)edge->m_ID))
                {
                    lookForWatchtower(edge, SQ, c->m_cost, updateKNN);
                    edge_visited.insert((void*)edge->m_ID);
                }
                int detid = edge->m_neighbor;
                Edge* edge_alternative = findEdgesinObjectSearch(detid, srcid, allnodes);
                if(!edge_visited.in((void*)edge_alternative->m_ID))
                {
                    lookForWatchtower(edge_alternative, SQ, c->m_cost, updateKNN, true);
                    edge_visited.insert((void*)edge_alternative->m_ID);
                }
            }

            const Array* objs = map.findObject(c->m_nid);

            int NumObjFound = checkObject(map, SQ, c);
            rescnt += NumObjFound;
            //----when we found some objects found, we may reduce candidates number in Qo
            updateKNN -=  rescnt;

            if(c->m_cost>expansion || rescnt >= KNN)
            {
                stop = true;
                break;
            }

            delete c;
        }


        bool print = false;
        rescnt = 0;
        SQ.getKNN(KNN,print);
        // ------------------------------------------------------------------------
        // clean up
        // ------------------------------------------------------------------------
        while (!h.isEmpty())
            delete (ObjectSearchResult*)h.removeTop();

        // ------------------------------------------------------------------------
        // all done
        // ------------------------------------------------------------------------
    }
    cout<<"we comapre watchtower add = "<< watchtowerAdd/node2query.size()<<endl;
    cout<<"we comapre watchtower Met = "<< watchtowerMet/node2query.size()<<endl;
    cout<<"we comapre at least watchtower's num that one object in WT = "<< atLeastOneObjWT/node2query.size()<<endl;

    watchtowerMet = 0;
    watchtowerAdd = 0;
}


void watchtowerSearch::hierSearch(Array& node2query, Array& a_allnodes, NodeMapping& map, vector<int>& Interval, float lambda)
{
    //-------------------------------------------------------------------------
    // initialization
    //-------------------------------------------------------------------------
//    LOG(INFO) <<"current KNN = "<<KNN<<endl;
    for(int query_i = 0; query_i<node2query.size(); query_i++)
    {
        NodeObject *nq = (NodeObject*) node2query.get(query_i);
        int nodeid = nq->m_nodeid;

    int src_ID = nodeid;
    int a_src = src_ID;
    bool stop=false;
    Set visited(1000);
    Set edge_visited(1000);
//    vector<QualifiedWatchtower> PQ;
    BinHeap h(ObjectSearchResult::compare);
    h.insert(new ObjectSearchResult(a_src,0));
    int layer = 0;
    searchQueue SQ;
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
        // --------------------------------------------------------------------
        // check if watchtower is found
        // --------------------------------------------------------------------

        int srcid = c->m_nid;
        // --------------------------------------------------------------------
        // first it search the current node, then search every edge
        // --------------------------------------------------------------------

        lookForWatchtower(node, SQ, c->m_cost, KNN);
        for (int e=0; e<node->m_edges.size(); e++)
        {
            Edge* edge = (Edge*)node->m_edges.get(e);
            if(!edge_visited.in((void*)edge->m_ID))
            {
                lookForWatchtower(edge, SQ, c->m_cost, KNN);
                edge_visited.insert((void*)edge->m_ID);
            }
            int detid = edge->m_neighbor;
            Edge* edge_alternative = findEdgesinObjectSearch(detid, srcid, a_allnodes);
            if(!edge_visited.in((void*)edge_alternative->m_ID))
            {
                lookForWatchtower(edge_alternative, SQ, c->m_cost, KNN, true);
                edge_visited.insert((void*)edge_alternative->m_ID);
            }
        }

        int NumObjFound = checkObject(map, SQ, c, layer);
        rescnt += NumObjFound;

        if(rescnt >= KNN)
            break;

        if(c->m_cost>Interval[layer]*lambda)
        {
            stop = JudgeStop(SQ,KNN,layer);
            layer++;
        }

        if(stop == true)
        {
//            cerr<<"next layer == "<<layer+1<<endl;
//            LOG(INFO) << " The layer " << layer  << " stops"<<endl;
            break;
        }

        delete c;

    }

    rescnt = 0;
    bool print = false;
    SQ.getKNN(KNN,print);

    while (!h.isEmpty())
        delete (ObjectSearchResult*)h.removeTop();
    SQ.clean();
    // ------------------------------------------------------------------------
    // all done
    // ------------------------------------------------------------------------
    }

    cout<<"we comapre watchtower add = "<< watchtowerAdd/node2query.size()<<endl;
    cout<<"we comapre watchtower Met = "<< watchtowerMet/node2query.size()<<endl;
    watchtowerMet = 0;
    watchtowerAdd = 0;
}

void watchtowerSearch::personalSearch(Array& allclusters, vector<int> & queryNodeID, Array& allnodes, NodeMapping& map, int select_hot, int select_non_hot)
{
    //-------------------------------------------------------------------------
    // initialization
    //-------------------------------------------------------------------------
    for(int query_i = 0; query_i<queryNodeID.size(); query_i++)
    {
        int nodeid = queryNodeID[query_i];
        Node* queryNode = (Node*)allnodes.get(nodeid);
        bool incluster = false;
        float expansion;
        for (int j=0; j<allclusters.size(); j++)
        {
            cluster* c1 = (cluster*)allclusters.get(j);
            double distance = sqrt( pow(c1->x- queryNode->m_x, 2 ) + pow(c1->y- queryNode->m_y, 2 ) );
//            cout<<"distance  = "<<distance<<" c1->r ="<<c1->r<<endl;
            if(distance < c1->r*20)
            {
                incluster = true;
                break;
            }

        }
        if(incluster == false)
            expansion = select_non_hot*lambda;
        else
            {
                cout<<" query point "<< query_i<<" is in the hot zones"<<endl;
                expansion = select_hot*lambda;
            }

        int a_src = nodeid;
        bool stop=false;
        Set visited(1000);

        Set edge_visited(1000);

        vector<QualifiedWatchtower> PQ;
        BinHeap h(ObjectSearchResult::compare);
        h.insert(new ObjectSearchResult(a_src,0));
        searchQueue SQ(true);

        //---------we may reduce KNN number when we found objects in the query SSSP expansion.
        int updateKNN = KNN;
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
            Node* node = (Node*)allnodes.get(c->m_nid);
            for (int e=0; e<node->m_edges.size(); e++)
            {
                Edge* edge = (Edge*)node->m_edges.get(e);
                h.insert(
                    new ObjectSearchResult(c->m_nid, edge->m_neighbor, c->m_path,
                    c->m_cost + edge->m_cost));
            }
            // --------------------------------------------------------------------
            // check if watchtower is found
            // --------------------------------------------------------------------

            int srcid = c->m_nid;
            // --------------------------------------------------------------------
            // first it search the current node, then search every edge
            // --------------------------------------------------------------------

            lookForWatchtower(node, SQ, c->m_cost, updateKNN);
            for (int e=0; e<node->m_edges.size(); e++)
            {
                Edge* edge = (Edge*)node->m_edges.get(e);
                if(!edge_visited.in((void*)edge->m_ID))
                {
                    lookForWatchtower(edge, SQ, c->m_cost, updateKNN);
                    edge_visited.insert((void*)edge->m_ID);
                }
                int detid = edge->m_neighbor;
                Edge* edge_alternative = findEdgesinObjectSearch(detid, srcid, allnodes);
                if(!edge_visited.in((void*)edge_alternative->m_ID))
                {
                    lookForWatchtower(edge_alternative, SQ, c->m_cost, updateKNN, true);
                    edge_visited.insert((void*)edge_alternative->m_ID);
                }
            }

            const Array* objs = map.findObject(c->m_nid);

            int NumObjFound = checkObject(map, SQ, c);
            rescnt += NumObjFound;
            //----when we found some objects found, we may reduce candidates number in Qo
            updateKNN -=  rescnt;

            if(c->m_cost>expansion || rescnt >= KNN)
            {
                stop = true;
                break;
            }

            delete c;
        }


        bool print = false;
        rescnt = 0;
        SQ.getKNN(KNN,print);
        // ------------------------------------------------------------------------
        // clean up
        // ------------------------------------------------------------------------
        while (!h.isEmpty())
            delete (ObjectSearchResult*)h.removeTop();

        // ------------------------------------------------------------------------
        // all done
        // ------------------------------------------------------------------------
    }
    cout<<"we comapre watchtower add = "<< watchtowerAdd/queryNodeID.size()<<endl;
    cout<<"we comapre watchtower Met = "<< watchtowerMet/queryNodeID.size()<<endl;
    cout<<"we comapre at least watchtower's num that one object in WT = "<< atLeastOneObjWT/queryNodeID.size()<<endl;

    watchtowerMet = 0;
    watchtowerAdd = 0;
}

