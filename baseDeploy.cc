#include "collection.h"
#include "node.h"
#include "edge.h"
#include "baseWatchtower.h"
#include "baseDeploy.h"
#include <math.h>
#include <stdlib.h>
#include <bitset>
#include <iostream>
#include <glog/logging.h>

using namespace std;

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;

float lambda=0;

bool first;
int WT_ID=0;
const bool debug = false;

Edge * findEdges(int src_node, int dest_node, Array& a_allnodes, int flag=0)
{
     if(flag ==0)
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
     cout<<"error in finding the other direction's edge"<<endl;
     return 0;
     }

     // for find the other direction's edge
     if(flag ==1)
     {
     Node* node = (Node*)a_allnodes.get(src_node);
//     cerr << "node_ID:    "<<node->m_id<<endl;
     int i=-1;
     Edge *otherDirection;
     if(node->m_edges.size() != 2)
     return 0;
     for(int j=0;j < node->m_edges.size(); j++)
     {
         otherDirection = (Edge*) node->m_edges[j];
         if(otherDirection->m_neighbor==dest_node)
         {
             i =j;
         }
     }
     otherDirection = (Edge*) node->m_edges[1-i];
     return otherDirection;
     }

}

float setOneEdge(Edge *edge, Edge *edge_alternative, float edge_cost, float remain)
{
      float temp_distance = lambda;
      float src_distance;
      int src_ID = edge_alternative->m_neighbor;
      int dest_ID = edge->m_neighbor;
      if(remain + edge_cost < lambda)
          return remain + edge_cost;
      else
      {
          while( temp_distance < edge_cost +remain)
          {
                 //add a watchtower
              src_distance = temp_distance - remain;
              baseWatchtower a_baseWatchtwoer(WT_ID, src_distance);
                WT_ID++;
              edge->addWatchtowers(a_baseWatchtwoer);
              baseWatchtower alternative_baseWatchtwoer(WT_ID, edge_cost-src_distance);
                WT_ID++;
              edge_alternative->addWatchtowers(alternative_baseWatchtwoer);
              temp_distance += lambda;
//              LOG(INFO) <<"edge = "<<edge-> m_ID;
//              LOG(INFO) <<"   edge_alternative = "<<edge_alternative-> m_ID<<endl;
          }
          return edge_cost +remain - temp_distance+ lambda;
      }

}


float updateOneEdge(Edge *edge, Edge *edge_alternative)
{
     if (edge->selected_watchtowers.size() >1)
     {
        edge->selected_watchtowers.sort(watchtowerObject::compare);
     }

     if (edge_alternative->selected_watchtowers.size() >1)
     {
        edge_alternative->selected_watchtowers.sort(watchtowerObject::compare);
     }
}

void setEdges(int node_ID, Edge *edge, float remain, Array& a_allnodes,  Array& a_alledges, vec_bool& bit_edge)
{
     if( bit_edge[edge->m_ID] ==true)
        return ;

     if(first==false)
     {
         Node* node = (Node*)a_allnodes.get(node_ID);
         if(node->m_edges.size() != 2)
         {
                  cerr << "finish to a 3 degree point:    "<<node ->m_id << endl;
         return ;
         }
     }
     first=false;
     int edge_ID = edge->m_ID;
     float edge_cost = edge->m_cost;
     int neighbor_ID = edge->m_neighbor;
//     cerr << "node_ID:    "<<node_ID<< "next_ID   " << neighbor_ID<< endl;
     Edge* edge_alternative;
     Edge* next_edge;
     bit_edge[edge->m_ID] =true;
     if(edge->m_ID %2 ==0)
     {
         bit_edge[edge_ID + 1] =true;
     }
     else
     {
         bit_edge[edge_ID - 1] =true;
     }
         edge_alternative = findEdges(neighbor_ID, node_ID, a_allnodes);
 //        cerr << "edge_alternative_ID:    "<<edge_alternative ->m_ID<< "edge_alternative_ next  ID   " << edge_alternative ->m_neighbor<< endl;

         remain = setOneEdge(edge, edge_alternative, edge_cost, remain);
         next_edge = findEdges(neighbor_ID, node_ID, a_allnodes, 1);

//         if(bit_edge[edge_1->m_ID] == true)
//             setEdges(neighbor_ID, next_edge, remain, a_allnodes, a_alledges, bit_edge);
//         else

         if(next_edge!=0)
         {
 //           cerr << "if li:    "<<remain<< endl;
            setEdges(neighbor_ID, next_edge, remain, a_allnodes, a_alledges, bit_edge);
         }

}

void updateEdges(int node_ID, Edge *edge, Array& a_allnodes, Array& a_alledges, vec_bool& bit_edge)
{
     if( bit_edge[edge->m_ID] ==true)
        return ;

     if(first==false)
     {
         Node* node = (Node*)a_allnodes.get(node_ID);
         if(node->m_edges.size() != 2)
         {
                  cerr << "finish to a 3 degree point:    "<<node ->m_id << endl;
         return ;
         }
     }
     first=false;
     int edge_ID = edge->m_ID;
     float edge_cost = edge->m_cost;
     int neighbor_ID = edge->m_neighbor;

     Edge* edge_alternative;
     Edge* next_edge;
     bit_edge[edge->m_ID] =true;
     if(edge->m_ID %2 ==0)
     {
         bit_edge[edge_ID + 1] =true;
     }
     else
     {
         bit_edge[edge_ID - 1] =true;
     }

     edge_alternative = findEdges(neighbor_ID, node_ID, a_allnodes);

     updateOneEdge(edge, edge_alternative);

     next_edge = findEdges(neighbor_ID, node_ID, a_allnodes, 1);

     if(next_edge!=0)
     {
        updateEdges(neighbor_ID, next_edge, a_allnodes, a_alledges, bit_edge);
     }

}

int baseDeploy::deploy( Array& a_allnodes, Array& a_alledges, float a_lambda)
{
     lambda = a_lambda;
     vec_int vec_node;
     int numnode = a_allnodes.size();
     int num_edges = a_alledges.size();
     for(int i=0;i<numnode;i++)
     {
         Node* node = (Node*)a_allnodes.get(i);
         if (node->m_edges.size() > 2)
         vec_node.push_back(i);
     }

     std::vector<bool> bit_edge;
     for(long i=0;i< num_edges; i++)
         bit_edge.push_back(false);
     // now, it is all 0 in bit_edge.
     for(int i=0;i < vec_node.size(); i++)
     {
         if(debug == true){
//         cerr << "vec_node:    "<< vec_node[i]<<"xia biao   "<<i <<"   vec_node.size(): "<< vec_node.size()<< endl;
         }
         Node* node = (Node*)a_allnodes.get(vec_node[i]);
         baseWatchtower a_baseWatchtwoer(WT_ID, 0);
         node->addWatchtowers(a_baseWatchtwoer);
         WT_ID++;
//         cerr << "   size:    "<< node->m_edges.size()<< endl;
         for(int j=0;j < node->m_edges.size(); j++)
         {
         float remain =0;
         Edge* edge = (Edge*) node->m_edges[j];
//         cerr << "      start:    "<<vec_node[i] << endl;
         first=true;
         setEdges(vec_node[i], edge, remain, a_allnodes, a_alledges, bit_edge);
//         cerr << "finish:    "<<vec_node[i] << endl;
         }

     }

     cerr << "the number of basewatchtower:    "<< (WT_ID - vec_node.size())/2 + vec_node.size()<<endl;

     return (WT_ID - vec_node.size())/2 + vec_node.size();

}




void baseDeploy::updateWatchtower( Array& a_allnodes, Array& a_alledges)
{
     vec_int vec_node;
     int numnode = a_allnodes.size();
     int num_edges = a_alledges.size();
     for(int i=0;i<numnode;i++)
     {
         Node* node = (Node*)a_allnodes.get(i);
         if (node->m_edges.size() > 2)
         vec_node.push_back(i);
     }

     std::vector<bool> bit_edge;
     for(long i=0;i< num_edges; i++)
         bit_edge.push_back(false);
     // now, it is all 0 in bit_edge.
     for(int i=0;i < vec_node.size(); i++)
     {
         if(debug == true){
            cerr << "vec_node:    "<< vec_node[i]<<"xia biao   "<<i <<"   vec_node.size(): "<< vec_node.size()<< endl;
         }
         Node* node = (Node*)a_allnodes.get(vec_node[i]);


         if (node->selected_watchtowers.size() >1)
         {
            node->selected_watchtowers.sort(watchtowerObject::compare);
         }

         for(int j=0;j < node->m_edges.size(); j++)
         {
             Edge* edge = (Edge*) node->m_edges[j];
             first=true;
             updateEdges(vec_node[i], edge, a_allnodes, a_alledges, bit_edge);
         }

     }

}

int baseDeploy::findHotzoneWT(Array& allnodes, Array& alledges)
{
//    for (int i=0; i<allnodes.size(); i++)
//    {
//        Node* node = (Node*)allnodes.get(i);
//        if
//    }
//
//    for (int i=0; i<alledges.size(); i++)
//    {
//        Edge* edge = (Edge*)alledges.get(i);
//        int nodeLeft = edge->m_srcID;
//        int nodeRight = edge->m_neighbor;
//
//        Node* node_left = (Node*)allnodes.get(nodeLeft);
//        Node* node_right = (Node*)allnodes.get(nodeRight);
//
//        if(node_left->getPersonal() &&  node_right->getPersonal()  )
//            {edge->setPersonal(true);count_hot_edge++;}
//    }

}
