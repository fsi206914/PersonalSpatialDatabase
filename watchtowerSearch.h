#ifndef watchtowerSearch_defined
#define watchtowerSearch_defined
#include "collection.h"
#include "nodemap.h"
#include <vector>
class watchtowerSearch
{

public:
    int KNN;
    int objectNum;
    int rescnt;
    double lambda;
    watchtowerSearch(int a_KNN, int a_objectNum, double a_lambda = 0)
    {
        KNN = a_KNN;
        objectNum = a_objectNum;
        rescnt = 0;
        lambda = a_lambda;
    }
    void oneLayerSearch(Array& node2query, Array& allnodes, NodeMapping& map, float expansion);
    void personalSearch(Array& allclusters, vector<int> &, Array& allnodes, NodeMapping& map, int, int);

    void hierSearch(Array& node2query, Array& allnodes, NodeMapping& map, vector<int>& Interval, float lambda);

};
#endif
