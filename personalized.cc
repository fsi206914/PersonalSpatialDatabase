/* ----------------------------------------------------------------------------
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
#include "baseDeploy.h"
//#include "graphplot.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <math.h>       /* pow   e */
#include <stdlib.h>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <sys/time.h>
#include <ANN/ANN.h> // ANN declarations
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

void restore_schedule(watchtowerArray &s, const char * filename)
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
    const char* every  = Param::read(a_argc, a_argv, "-hr", "");
    const char* nonevery  = Param::read(a_argc, a_argv, "-nhr", "");
    const char* threshould  = Param::read(a_argc, a_argv, "-th", "");

    const char* storage  = Param::read(a_argc, a_argv, "-st", "");

    const char* charKNN  = Param::read(a_argc, a_argv, "-k", "");

    const char* vrbs = Param::read(a_argc, a_argv, "-v", "null");


    bool verbose = strcmp(vrbs,"null") != 0;
    string dataSetName = string (nodeflname); bool NA = false;
    if(dataSetName == "NA.cnode")
        NA = true;
    fstream fnode, fedge, fmean, fvar, fcheck_in;
    clock_t begin_time;
    Array node2query;           // a mapping between nodes and query nodes
    Array node2object;
    Hash nodes(NUM_NODES);          // this is for fast quick lookup
    Array allnodes(NUM_NODES,10000);  // this is for node ordering
    Array alledges(50000,100);  // this is for node ordering
    Array allclusters;  // this is for node ordering

    float prob_thsd = atof(threshould);
    vector<int> queryNodeID;
    ANNpointArray	data_pts = annAllocPts(200000, 2);				// data points
    ANNpointArray	query_pts = annAllocPts(50, 2);				// data points

    ANNidxArray nnIdx = new ANNidx[1];
    ANNdistArray dists = new ANNdist[1];


    vector<int> newAllnodes;  // this is for node ordering

    struct timespec requestStart, requestEnd; // for calculating time
    double accum; // the Time duration
    watchtowerArray *WA = new watchtowerArray();


    cerr << "loading nodes ... ";
    fnode.open(nodeflname, ios::in);
    int countNode = 0;
    int dim = 2;

    while (true)
    {
        int id;
        float x,y, temp;
        fnode >> id;
        if (fnode.eof())
            break;

        fnode >> x;
        fnode >> y;

        if(NA == true)
        {
            y = 10.0+ (float)(40.0/10000.0)*x;
            x = -130.0+ (float)(77.26/10000.0)*y;
        }

        // x is longitude, y is latitude
        Node* node = new Node(id, x, y, 0 ,WA);
        //--- I want o know what hash save in memory? address or node?
        //--- Answer is the address. Append only save the address of the pointer.
        nodes.put(id,node);
        allnodes.append(node);
        data_pts[id][0] = (double)x;
        data_pts[id][1] = (double)y;

        countNode++;
    }
    allnodes.sort(Node::compareid);
    cerr << "[DONE]" << endl;
    fnode.close();

    ANNkd_tree* kdTree = new ANNkd_tree(
    data_pts,
    countNode,
    2);

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

        Edge *e1 = new Edge(dest, cost, edge_ID, WA, src);
        alledges.append(e1);
        edge_ID++;
        Node* srcnode = (Node*)nodes.get(src);
        srcnode->addEdge(*e1);

        Edge *e2 = new Edge(src, cost, edge_ID, WA, src);
        alledges.append(e2);
        edge_ID++;
        Node* destnode= (Node*)nodes.get(dest);
        destnode->addEdge(*e2);
    }
    cerr << "[DONE]" << endl;
    fnode.close();
    fedge.close();

//    //-------------------------------------------------------------------------
//    // If it is in the CA dataset, it needs smaller lambda; while in NA, it needs larger one.
//    // Then we deploy base watchtowers in the graph based on lambda
//    //-------------------------------------------------------------------------
    float lambda;

    if(edge_ID>100000) lambda=5;
        else lambda = 0.01;

    int basewatchtower_num = baseDeploy::deploy(allnodes, alledges, lambda);

    //-------------------------------------------------------------------------
    // get new G
    //-------------------------------------------------------------------------
    fmean.open("./clusterInfo/NA_mean_20", ios::in);
    fvar.open("./clusterInfo/NA_var_20", ios::in);

    int num_cluster =0;
//    prob_thsd = 0.0001;

    while (true)
    {
        float x, y;
        float var_1, var_2, var_3, var_4;
        fmean >> x;
        if (fmean.eof())
            break;
        fmean >> y;
        fvar >>var_1 >> var_2 >>var_3 >>var_4 ;
//        var_4 = 0.2;
        double equ1 = prob_thsd * 2 *3.14159 * pow(var_4, 2);
        double equ2 = log(equ1);
        double r = sqrt(equ2 * (-2 * pow(var_4,2)));
//        r = 10.0;
        cluster *c1 = new cluster(num_cluster, x, y, var_4, r);
        allclusters.append(c1);
        num_cluster++;

    }
    for (int j=0; j<allclusters.size(); j++)
    {
        cluster* c1 = (cluster*)allclusters.get(j);
        cout<<" c1-> r = "<< c1-> r<<endl;
        break;
    }
    //-------------------------------------------------------------------------
    // test what the percentage of check-in point is in cluster
    //-------------------------------------------------------------------------
    fcheck_in.open("result.txt", ios::in);
    int inCluster =0;     int outCluster =0;
    bool incluster = false;
    srand (time(NULL));

    int LimitQueryNum = 50;
    int query_num = 0;
    while (true)
    {

        double x, y;
        fcheck_in >> y;
        if (fcheck_in.eof())
            break;
        fcheck_in >> x;
        incluster = false;
        for (int j=0; j<allclusters.size(); j++)
        {
            cluster* c1 = (cluster*)allclusters.get(j);
            double distance = sqrt( pow(c1->x- x, 2 ) + pow(c1->y- y, 2 ) );
//            cout<<"node->m_x = "<< x << " node->m_y ="<< y<<"  distance = "<<distance <<"  c1->r = "<<c1->r<<endl;
            if(distance < c1->r)
            {
                inCluster++;
                incluster = true;
                break;
            }

        }
        if(incluster == false)
        {
            outCluster++;
        }
        int iSecret = rand() % 1000;
        double prob = (double)iSecret/1000;
        if(prob <0.008 && query_num< LimitQueryNum)
        {
            query_pts[query_num][0] = (double)x;
            data_pts[query_num][1] = (double)y;
            query_num++;
        }

    }
    cerr << "the num of check in location inside clusters : "<<inCluster  << endl;
    cerr << "the num of check in location outside clusters : "<<outCluster  << endl;


    fmean.close();
    fvar.close();
    fcheck_in.close();

    cerr << "creating new G" << endl;

    int count_hot_node=0; int count_hot_edge = 0;
    int hotZoneWatchtower = 0; int edgeHotWT = 0; //edgeHotWT needs to be cut down half.


    float power_e, result;
    bool nextNode = false;
    for (int i=0; i<allnodes.size(); i++)
    {
        Node* node = (Node*)allnodes.get(i);
        for (int j=0; j<allclusters.size(); j++)
        {
            cluster* c1 = (cluster*)allclusters.get(j);
            double distance = sqrt( pow(c1->x- node->m_x, 2 ) + pow(c1->y- node->m_y, 2 ) );
//            cout<<"node->m_x = "<< c1->x << " node->m_y ="<< c1->y<<"  distance = "<<distance <<"  c1->r = "<<c1->r<<endl;
            if(distance < c1->r)
            {
                node->setPersonal(true);count_hot_node++;
                if(node->m_watchtowers.size()>0)
                    hotZoneWatchtower += 1;
                break;
            }
        }

    }

    for (int i=0; i<allnodes.size(); i++)
    {
        Node* node = (Node*)allnodes.get(i);

        for (int j=0; j<node->m_edges.size(); j++)
        {
            Edge* edge = (Edge*)node->m_edges.get(j);
            Node* node_left = (Node*)allnodes.get(edge->m_srcID);
            Node* node_right = (Node*)allnodes.get(edge->m_neighbor);

            if(node_left->getPersonal() &&  node_right->getPersonal()  )
            {
                edge->setPersonal(true);count_hot_edge++;
                edgeHotWT += edge->m_watchtowers.size();
            }
        }
    }

    cout<<" hot zone node #= "<<count_hot_node<<"   hot zone edge # ="<< count_hot_edge<<endl;
    cout<<" hot zone node WT #= "<<hotZoneWatchtower<<"   hot zone edge WT # ="<< edgeHotWT/2<<endl;

    hotZoneWatchtower += edgeHotWT/2;
//    //-------------------------------------------------------------------------
//    // saving graph to a file
//    //-------------------------------------------------------------------------
//
////    SegFMemory segmem(idxflname,PAGESIZE*10,PAGESIZE,32,true);    // 10 pages, 1 page
////    Graph graph(segmem);
////    for (int i=0; i<allnodes.size(); i++)
////    {
////        Node* node = (Node*)allnodes.get(i);
////        if (node->m_edges.size() > 0)
////            graph.writeNode(node->m_id,*node);
////    }
////    cerr << "graph write [DONE]" << endl;


    //-------------------------------------------------------------------------
    // creating query node using kNN from a check-in location
    //-------------------------------------------------------------------------
    while (query_num >= 1) { // read query points

        kdTree->annkSearch( //
            query_pts[query_num-1], //
            1, //
            nnIdx, //
            dists, //
            0); //


//    cout << "NN: Index Distance\n"; // print summary
    for (int i = 0; i < 1; i++) { // unsquare distance
    dists[i] = sqrt(dists[i]);
//    cout << i << " " << nnIdx[i]<<" "<< dists[i] << "\n";
    queryNodeID.push_back(nnIdx[i]);
    }
    query_num--;
    }

//--------------------create objects--------------------//
    int numobj =atol(numobject);

    double stoMb;
    if(edge_ID>100000)
       stoMb = atof(storage) -7.8;
    else
       stoMb = atof(storage) -0.9426;



    int everyInterval, hotzoneInterval;
        string everyString = string (every);
    if(everyString == "")
    {
        double object_per_size = (double)stoMb/(double)numobj;
        cout<<" object_per_size = "<<object_per_size<<endl;
        int watchtower_should_select = (int)( object_per_size*( (double)outCluster/( (double)inCluster+(double)outCluster)) *10000/0.22); // o.3 means non hot zone needs only 30 percent of storage.
        cout<<" watchtower_should_select in non hot zone = "<<watchtower_should_select<<endl;
        int select_l = (basewatchtower_num - hotZoneWatchtower)/watchtower_should_select +1;
        everyInterval = select_l-1;

        watchtower_should_select = (int)( object_per_size*( (double)inCluster/ ((double)inCluster+(double)outCluster))*10000/0.22);
        cout<<" watchtower_should_select in hot zone = "<<watchtower_should_select<<endl;
        select_l = hotZoneWatchtower/watchtower_should_select +1;
        hotzoneInterval = select_l-1;
    }
    else
    {
        everyInterval = atol(every)-1;
        hotzoneInterval = atol(nonevery)-1;
        hotzoneInterval = hotzoneInterval > 20? hotzoneInterval:20 ;
    }

    cout<<"every "<<everyInterval+1<<" we choose one watchtower in non-hot zone"<<endl;
    cout<<"every "<<hotzoneInterval+1<<" we choose one watchtower in hot zone"<<endl;

    ObjectGen::uniform(numobj, allnodes, node2object);

//-------------Save Info Part----index creating
    cerr << "For every object, watchtowers are set up ... "<<endl;
    //-------------------------------------------------------------------------
    // The most accurate appraoch to test period time by nano second in GCC
    //-------------------------------------------------------------------------
    clock_gettime(CLOCK_REALTIME, &requestStart);

    long allSelected = saveInfo::personalSave(node2object, allnodes, everyInterval, hotzoneInterval);

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



//------test for one layer watchtower structure

    clock_gettime(CLOCK_REALTIME, &requestStart);
////-----------------search creating

    int kNN =atol(charKNN);
    int select_hot = hotzoneInterval;
    cout<<"KNN's K = "<<kNN<<endl;
    watchtowerSearch wSearch(kNN, numobj,lambda);
    wSearch.personalSearch(allclusters,queryNodeID,allnodes,map,select_hot,everyInterval);

    clock_gettime(CLOCK_REALTIME, &requestEnd);
    accum = ( requestEnd.tv_sec - requestStart.tv_sec ) + ( requestEnd.tv_nsec - requestStart.tv_nsec )/ BILLION;
    printf( "query time = %lf s\n", accum/queryNodeID.size() );



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
