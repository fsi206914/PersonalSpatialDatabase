/* ----------------------------------------------------------------------------
    Author: Ken C. K. Lee
    Email:  cklee@cse.psu.edu
    Web:    http://www.cse.psu.edu/~cklee
    Date:   Jan, 2008

    Copyright(c) Ken C. K. Lee 2008
    This program is for non-commerical use only.

    This program
    * builds index for a graph and
    * generates (random) objects for a graph.

    Suggested arguments:
    > (prog name)
      -n nodefile.txt -e edgefile.txt
      -m #objects -c #clusters -s #query
      -i graph.idx -o object.dat -q query.dat
      -v
    explanations:
    -n: node file
    -e: edge file
    -m: #objects
    -c: #clusters
    -s: #query points
    -i: graph index file
    -o: object file (a list of nodeid and object id)
    -q: query node file (a list of nodeid and query id)
    -p: file that prints the graph in Postscript
    -v: turn verbose mode on (default: off)
---------------------------------------------------------------------------- */
#include "graph.h"
#include "node.h"
#include "edge.h"
#include "saveInfo.h"
#include "segfmem.h"
#include "nodemap.h"
#include "objectgen.h"
#include "objectsearch.h"
#include "watchtowerSearch.h"
#include "param.h"
#include "collection.h"
#include <glog/logging.h>

#include "baseDeploy.h"
//#include "graphplot.h"
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <sys/time.h>
#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/utility.hpp>


using namespace std;

#define MAXNODES    100000
#define PAGESIZE    4096    // 4KB page
#define NUM_NODES    21048    // 4KB page
#define BILLION 1E9

void helpmsg(const char* pgm)
{
    cerr << "Suggested arguments:" << endl;
    cerr << "> " << pgm << " ";
    cerr << "-n nodefile.txt -e edgefile.txt -m #objects -c #clusters ";
    cerr << "-i graph.idx -o object.dat -p output.ps -v" << endl;
    cerr << "explanations:" << endl;
    cerr << "-n: node file" << endl;
    cerr << "    format: nodeID  x y " << endl;
    cerr << "-e: edge file" << endl;
    cerr << "    format: src_nodeID dest_nodeID cost" << endl;
    cerr << "-m: number of objects" << endl;
    cerr << "-c: number of clusters" << endl;
    cerr << "-s: number of query points" << endl;
    cerr << "-i: graph index file (output)" << endl;
    cerr << "-o: object file [nodeid, object id] (output)" << endl;
    cerr << "-q: query file [nodeid, query id] (output)" << endl;
    cerr << "-p: file that prints the graph in Postscript" << endl;
    cerr << "-r: how many watchtower it chooses one" << endl;
    cerr << "-k: K nearest neighbor defined" << endl;
    cerr << "-v: turn verbose mode on (default: off)" << endl;
}


void save_schedule(const watchtowerArray &s, const char * filename){
    // make an archive
    std::ofstream ofs(filename);
    boost::archive::text_oarchive  oa(ofs);
    oa << s;
}

void
restore_schedule(watchtowerArray &s, const char * filename)
{
    // open the archive
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);

    // restore the schedule from the archive
    ia >> s;
}

int main(const int a_argc, const char** a_argv)
{
    if (a_argc == 1)
    {
        helpmsg(a_argv[0]);
        return -1;
    }
    google::InitGoogleLogging(a_argv[0]);

    cerr << "graph, object and query loader" << endl;
    //-------------------------------------------------------------------------
    const char* nodeflname = Param::read(a_argc, a_argv, "-n", "");
    const char* edgeflname = Param::read(a_argc, a_argv, "-e", "");
    const char* numobject  = Param::read(a_argc, a_argv, "-m", "");
    const char* numcluster = Param::read(a_argc, a_argv, "-c", "");
    const char* numquery = Param::read(a_argc, a_argv, "-s", "");
    const char* idxflname = Param::read(a_argc, a_argv, "-i", "");
    const char* objflname = Param::read(a_argc, a_argv, "-o", "");
    const char* qryflname = Param::read(a_argc, a_argv, "-q", "");
    const char* psflname = Param::read(a_argc, a_argv, "-p", "");
    const char* every  = Param::read(a_argc, a_argv, "-r", "");
    const char* charKNN  = Param::read(a_argc, a_argv, "-k", "");

    const char* vrbs = Param::read(a_argc, a_argv, "-v", "null");
    bool verbose = strcmp(vrbs,"null") != 0;

    fstream fnode, fedge;
    clock_t begin_time;
    Array node2query;           // a mapping between nodes and query nodes
    Array node2object;
    Hash nodes(NUM_NODES);          // this is for fast quick lookup
    Array allnodes(NUM_NODES,10000);  // this is for node ordering
    Array alledges(50000,100);  // this is for node ordering
    struct timespec requestStart, requestEnd; // for calculating time
    double accum; // the Time duration
    watchtowerArray *WA = new watchtowerArray();

    //-------------------------------------------------------------------------
    // loading nodes and edges
    //-------------------------------------------------------------------------
    cerr << "loading nodes ... ";
    fnode.open(nodeflname, ios::in);
    while (true)
    {
        int id;
        float x,y;
        fnode >> id;
        if (fnode.eof())
            break;

        fnode >> x;
        fnode >> y;

        Node* node = new Node(id,x,y, 0 ,WA);
        //--- I want o know what hash save in memory? address or node?
        //--- Answer is the address. Append only save the address of the pointer.
        nodes.put(id,node);
        allnodes.append(node);
    }
    allnodes.sort(Node::compareid);
    cerr << "[DONE]" << endl;

    cerr << "loading edges ... ";
    fedge.open(edgeflname, ios::in);

    long edge_ID =0;
    while (true)
    {
        int id;
        int src, dest;
        float cost;
        fedge >> id;
        if (fedge.eof())
            break;
        fedge >> src;
        fedge >> dest;
        fedge >> cost;

        Edge *e1 = new Edge(dest, cost, edge_ID, WA);
        alledges.append(e1);
        edge_ID++;
        Node* srcnode = (Node*)nodes.get(src);
        srcnode->addEdge(*e1);

        Edge *e2 = new Edge(src, cost, edge_ID, WA);
        alledges.append(e2);
        edge_ID++;
        Node* destnode= (Node*)nodes.get(dest);
        destnode->addEdge(*e2);
    }
    cerr << "[DONE]" << endl;
    fnode.close();
    fedge.close();

    //-------------------------------------------------------------------------
    // saving graph to a file
    //-------------------------------------------------------------------------

//    SegFMemory segmem(idxflname,PAGESIZE*10,PAGESIZE,32,true);    // 10 pages, 1 page
//    Graph graph(segmem);
//    for (int i=0; i<allnodes.size(); i++)
//    {
//        Node* node = (Node*)allnodes.get(i);
//        if (node->m_edges.size() > 0)
//            graph.writeNode(node->m_id,*node);
//    }
//    cerr << "graph write [DONE]" << endl;


    //-------------------------------------------------------------------------
    // If it is in the CA dataset, it needs smaller lambda; while in NA, it needs larger one.
    // Then we deploy base watchtowers in the graph based on lambda
    //-------------------------------------------------------------------------

    float lambda;
    vector<int> Interval;
    vector<double> radius;

    if(edge_ID>100000)
    {
        lambda=5;
        double mydouble[] = {1000.0,2000.0,3000.0, 4000.0, 5000.0, 6000.0, 7000.0, 8000.0, 9000.0, 10000.0, 12000.0, 14000.0, 16000.0};
        int myints[] = {60,120,150,200,250,300,250,400,450,500, 500, 600, 600};
        Interval = vector<int> (myints, myints + sizeof(myints) / sizeof(int) );
        radius = vector<double> (mydouble, mydouble + sizeof(mydouble) / sizeof(double) );
    }
    else
    {
        lambda = 0.01;
        double mydouble[] = {0.5, 1.0, 4.0, 8.0, 16.0, 12.0, 14.0, 16.0, 18.0, 20.0};
        int myints[] = {20,50,100,150,200,180,200,80,90,100};
        Interval = vector<int> (myints, myints + sizeof(myints) / sizeof(int) );
        radius = vector<double> (mydouble, mydouble + sizeof(mydouble) / sizeof(double) );
    }
    int basewatchtower_num = baseDeploy::deploy(allnodes, alledges, lambda);

//--------------------test class's size

//    cerr << "sizeof(Node) = " <<sizeof(Node)<< endl;
//    cerr << "sizeof(baseWatchtower) = " <<sizeof(baseWatchtower)<< endl;
//    cerr << "sizeof(watchtowerObject) = " <<sizeof(watchtowerObject)<< endl;


//--------------------create objects--------------------//
    int numobj =atol(numobject);

    ObjectGen::uniform(numobj, allnodes, node2object);


    //-------------------------------------------------------------------------
    // -Save Info Part----index creating
    // The most accurate appraoch to test period time by nano second in GCC
    //-------------------------------------------------------------------------

    clock_gettime(CLOCK_REALTIME, &requestStart);

    long allSelected = saveInfo::hierObjectSave(node2object, allnodes, Interval, radius);


    //-------------------------------------------------------------------------
    // Then se sort every tuple in the every watchtower's site
    //-------------------------------------------------------------------------
    baseDeploy::updateWatchtower(allnodes, alledges);


    clock_gettime(CLOCK_REALTIME, &requestEnd);
    accum = ( requestEnd.tv_sec - requestStart.tv_sec ) + ( requestEnd.tv_nsec - requestStart.tv_nsec )/ BILLION;
    printf( "index Time : %lf s\n", accum );
    cout<<allSelected<<"  watchtowers are"<<" selected"<<endl;
    if(edge_ID>100000)
        cout<< (float)allSelected/10000*0.22+7.8 << "Mb" <<" are used"<<endl;
    else
        cout<< (float)allSelected/10000*0.22+0.9426 << "Mb" <<" are used"<<endl;

    //-------------------------------------------------------------------------
    // create Index file
    //-------------------------------------------------------------------------
//    std::string filename(boost::archive::tmpdir());
//    filename += "/serial";

//    // save the schedule
//    save_schedule(*WA, filename.c_str());

    //-------------------------------------------------------------------------
    // create objects in the graph
    //-------------------------------------------------------------------------
    NodeMapping map;

    for(int i =0;i<numobj; i++)
    {
        int nodeid, objid;

        NodeObject *nd = (NodeObject*) node2object.get(i);
        nodeid = nd->m_nodeid;
        objid = nd->m_objid;
//        cerr<<"node_ID :  "<<nodeid<<"    object ID: "<<objid<<endl;

        map.addObject(nodeid, objid);
    }
    //-------------------------------------------------------------------------
    // create querying nodes
    //-------------------------------------------------------------------------
    cerr << "create querying nodes ... "<<endl;;
    int numqry = 20;
    int kNN =atol(charKNN);
    ObjectGen::uniform(numqry, allnodes, node2query);


//------test for hier layer watchtower structure
    clock_gettime(CLOCK_REALTIME, &requestStart);
////-----------------search creating
    watchtowerSearch wSearch(kNN, numobj);
    wSearch.hierSearch(node2query,allnodes,map,Interval,lambda);

    clock_gettime(CLOCK_REALTIME, &requestEnd);
    accum = ( requestEnd.tv_sec - requestStart.tv_sec ) + ( requestEnd.tv_nsec - requestStart.tv_nsec )/ BILLION;
    printf( "query time = %lf s\n", accum/numqry );

    string dataSetName = string (nodeflname);
    LOG(INFO)<<"in "<<dataSetName<<"   In hier layer search:\n object num ="<<numobj<<" k = "<<kNN <<" query time = "<<accum/numqry<<endl;



    cerr << "everyting complete ... "<<endl;
    //-------------------------------------------------------------------------
    // clean up
    //-------------------------------------------------------------------------
    for (int i=0; i<allnodes.size(); i++)
        delete (Node*)allnodes.get(i);
    for (int i=0; i<node2object.size(); i++)
        delete (Node*)node2object.get(i);
    for (int i=0; i<node2query.size(); i++)
        delete (Node*)node2query.get(i);


    return 0;
}

