#include "node.h"
#include "edge.h"
#include "objectgen.h"
#include "saveInfo.h"
#include "baseWatchtower.h"
#include "objectsearch.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <bitset>
#include <iostream>
#include <proc/readproc.h>

using namespace std;

int count_selected = 0;
long allSelected = 0;

const bool debug = false;
    //-------------------------------------------------------------------------
    // look for the reverse edge based on src and dest node
    //-------------------------------------------------------------------------
Edge * findEdges(int src_node, int dest_node, Array& a_allnodes)
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

    //-------------------------------------------------------------------------
    // find watchtowers based on the interval.
    //-------------------------------------------------------------------------

int setInfoinEdge(Edge *edge, Node *det, int required_interval,int object_ID, int current_cost, int current_layer)
{
//    if(current_cost > 5)
//    required_interval = 1;
    int interval = det->remainInterval;
    int wtSize = edge->m_watchtowers.size();
    float edge_cost = edge->m_cost;

    if(wtSize<1) return interval;
    else
    {
        for(int i=edge->m_watchtowers.size()-1; i>=0; i--)
        {
            if(interval >= required_interval)
            {
                baseWatchtower *bw = (baseWatchtower*) edge->m_watchtowers[i];
                float temp = edge_cost - bw->m_src_cost + current_cost;

                edge->addSelected(object_ID, temp, bw->m_src_cost, current_layer+1);
                if(debug == true)
                {
                    cout <<" layer = "<< current_layer+1<<"cost = "<<temp <<endl;
                    cout <<" edge_cost = " <<edge_cost<<endl;
                    cout <<" bw->m_src_cost = " <<bw->m_src_cost<<endl;
                    cout <<" current_cost = " <<current_cost<<endl;
                }
                interval = -1;
                count_selected++;

            }
            interval++;
        }

        return interval;
    }
}


long saveInfo::objectSave(Array& node2object , Array& a_allnodes, int everyInterval)
{
    for(int i=0;i<node2object.size();i++)
    {
        NodeObject *object = (NodeObject *) node2object.get(i);
        int src_ID = object->m_nodeid;
        int object_ID = object->m_objid;
        cerr<<"object_ID: "<<object_ID<<"  src_ID: "<<src_ID<<endl;
    //-------------------------------------------------------------------------
    // initialization
    //-------------------------------------------------------------------------
    int rescnt = 0;
    int a_src = src_ID;
    int required_interval = everyInterval;

    //-------------------------------------------------------------------------
    // Dijkstra's shortest path search (best first)
    //-------------------------------------------------------------------------
    Set visited(1000);
    BinHeap h(ObjectSearchResult::compare);
    h.insert(new ObjectSearchResult(a_src,0));
    bool first = true;
//    cerr<<"a_src: "<<a_src<<endl;
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


// This edge will be the edge in the shortest path and operated. Then
// Watchtowers will be set up in the edge

        if(first == false)
        {
            int srcid = c->m_srcid;
            int detid = c->m_nid;
            int return_interval;

            Edge* edge_alternative;
            edge_alternative = findEdges(detid, srcid, a_allnodes);

            Node* node_src = (Node*)a_allnodes.get(srcid);

            return_interval = setInfoinEdge(edge_alternative, node_src, required_interval, object_ID, c->m_cost, 0);

            Node* node_det = (Node*)a_allnodes.get(detid);

            if( node_det->m_watchtowers.size() > 0)
            {
                if(return_interval >= required_interval){
                    node_det->addSelected(object_ID, edge_alternative->m_cost+c->m_cost, 0, 1);
                    return_interval = 0;
                    count_selected++;
                }
            }
//----------we must save remaining interval
//----------in the end of the edge expanded.
            node_det->remainInterval = return_interval;
        }

        first = false;
        visited.insert((void*)c->m_nid);

        Node* node = (Node*)a_allnodes.get(c->m_nid);

        for (int e=0; e<node->m_edges.size(); e++)
        {
            Edge* edge = (Edge*)node->m_edges.get(e);
            h.insert(
                new ObjectSearchResult(c->m_nid, edge->m_neighbor, c->m_path,
                c->m_cost + edge->m_cost));
        }

        delete c;

    }
//    cout<< "---How many base watchtowers selected: "<<count_selected<<endl;
    allSelected += count_selected;
    count_selected=0;
    while (!h.isEmpty())
        delete (ObjectSearchResult*)h.removeTop();

    h.clean();
    }
    return allSelected;

}

long saveInfo::hierObjectSave(Array& node2object , Array& a_allnodes, vector<int>& Interval, vector<double>& radius)
{
    for(int i=0;i<node2object.size();i++)
    {

        NodeObject *object = (NodeObject *) node2object.get(i);
        int src_ID = object->m_nodeid;
        int object_ID = object->m_objid;
        //-------------------------------------------------------------------------
        // initialization
        //-------------------------------------------------------------------------
        int rescnt = 0;
        int a_src = src_ID;

        int layer = 0;//Layer starts from 0;
//    Node* node = (Node*)a_allnodes.get(5262)
        cout<<"node: "<<src_ID<<endl;
        //-------------------------------------------------------------------------
        // Dijkstra's shortest path search (best first)
        //-------------------------------------------------------------------------
        Set visited(1000);
        BinHeap h(ObjectSearchResult::compare);
        h.insert(new ObjectSearchResult(a_src,0));
        bool first = true;
//    cerr<<"a_src: "<<a_src<<endl;
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


        // This edge will be the edge in the shortest path and operated. Then
        // Watchtowers will be set up in the edge
        if(first == false)
        {
            if(c->m_cost >radius[layer])
                {layer++;
			//cout<<"current layer"<<layer<<endl;
		}
            int srcid = c->m_srcid;
            int detid = c->m_nid;
            int return_interval;

            Edge* edge_alternative;
            edge_alternative = findEdges(detid, srcid, a_allnodes);

            Node* node_src = (Node*)a_allnodes.get(srcid);

            return_interval = setInfoinEdge(edge_alternative, node_src, Interval[layer], object_ID, c->m_cost, layer);

            Node* node_det = (Node*)a_allnodes.get(detid);

            if( node_det->m_watchtowers.size() > 0)
            {
                if(return_interval >= Interval[layer]-1){
                    node_det->addSelected(object_ID, edge_alternative->m_cost+c->m_cost, 0, layer+1);
                    return_interval = 0;
//                    if(debug == true) cout <<" layer = "<< layer+1<<"cost = "<<edge_alternative->m_cost+c->m_cost <<endl;
                    count_selected++;
                }
            }
        //----------we must save remaining interval
        //----------in the end of the edge expanded.
            node_det->remainInterval = return_interval;
        }

        first = false;
        visited.insert((void*)c->m_nid);

        Node* node = (Node*)a_allnodes.get(c->m_nid);

        for (int e=0; e<node->m_edges.size(); e++)
        {
            Edge* edge = (Edge*)node->m_edges.get(e);
            h.insert(
                new ObjectSearchResult(c->m_nid, edge->m_neighbor, c->m_path,
                c->m_cost + edge->m_cost));
        }
        delete c;

    }
//    cout<< "---How many base watchtowers selected: "<<count_selected<<endl;
    allSelected += count_selected;
    cout<<count_selected<<"  watchtowers are"<<" selected for one object"<<endl;

    count_selected=0;
    while (!h.isEmpty())
        delete (ObjectSearchResult*)h.removeTop();

    h.clean();
    }
    return allSelected;
}



long saveInfo::personalSave(Array& node2object, Array& a_allnodes, int everyInterval, int hotzoneInterval)
{
    for(int i=0;i<node2object.size();i++)
    {
        NodeObject *object = (NodeObject *) node2object.get(i);
        int src_ID = object->m_nodeid;
        int object_ID = object->m_objid;
        cerr<<"object_ID: "<<object_ID<<"  src_ID: "<<src_ID<<endl;
    //-------------------------------------------------------------------------
    // initialization
    //-------------------------------------------------------------------------
    int rescnt = 0;
    int a_src = src_ID;
    int required_interval = everyInterval;

    //-------------------------------------------------------------------------
    // Dijkstra's shortest path search (best first)
    //-------------------------------------------------------------------------
    Set visited(1000);
    BinHeap h(ObjectSearchResult::compare);
    h.insert(new ObjectSearchResult(a_src,0));
    bool first = true;
//    cerr<<"a_src: "<<a_src<<endl;
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


// This edge will be the edge in the shortest path and operated. Then
// Watchtowers will be set up in the edge

        if(first == false)
        {
            int srcid = c->m_srcid;
            int detid = c->m_nid;
            int return_interval;

            Edge* edge_alternative;
            edge_alternative = findEdges(detid, srcid, a_allnodes);

            if(edge_alternative->getPersonal())
            {
                required_interval = hotzoneInterval;
            }
            else
                required_interval = everyInterval;


            Node* node_src = (Node*)a_allnodes.get(srcid);

            return_interval = setInfoinEdge(edge_alternative, node_src, required_interval, object_ID, c->m_cost, 0);

            Node* node_det = (Node*)a_allnodes.get(detid);

            if( node_det->m_watchtowers.size() > 0)
            {
                if(edge_alternative->getPersonal())
                {
                    required_interval = hotzoneInterval;
                }

                if(return_interval >= required_interval){
                    node_det->addSelected(object_ID, edge_alternative->m_cost+c->m_cost, 0, 1);
                    return_interval = 0;
                    count_selected++;
                }
            }
//----------we must save remaining interval
//----------in the end of the edge expanded.
            node_det->remainInterval = return_interval;
        }

        first = false;
        visited.insert((void*)c->m_nid);

        Node* node = (Node*)a_allnodes.get(c->m_nid);

        for (int e=0; e<node->m_edges.size(); e++)
        {
            Edge* edge = (Edge*)node->m_edges.get(e);
            h.insert(
                new ObjectSearchResult(c->m_nid, edge->m_neighbor, c->m_path,
                c->m_cost + edge->m_cost));
        }

        delete c;

    }
//    cout<< "---How many base watchtowers selected: "<<count_selected<<endl;
    allSelected += count_selected;
    count_selected=0;
    while (!h.isEmpty())
        delete (ObjectSearchResult*)h.removeTop();

    h.clean();
    }
    return allSelected;

}
