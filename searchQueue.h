
#ifndef searchQueue_defined
#define searchQueue_defined

#include<vector>
#include<iostream>
#include<map>
#include<iostream>
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
    QualifiedWatchtower(int a_object_ID, float a_current_cost, int a_level, float a_farFromObject )
    {
        object_ID = a_object_ID; current_cost = a_current_cost; level = a_level; farFromObject = a_farFromObject;
    };

    QualifiedWatchtower()
    {};

}QualifiedWatchtower, *QualifyPointer;

bool compareByLength(const QualifyPointer &a, const QualifyPointer &b)
{
    return a->current_cost+a->farFromObject < b->current_cost+b->farFromObject ;
}


class searchQueue
{
public:
    vector<QualifyPointer>    s_id2node;
    map<int, int>   s_nodeMap; // <object, array No.>
    int current_layer;
    int Candidates;
    bool onelayer;
public:
    // constructor/destructor
    searchQueue(const bool a_onelayer = false)
    {
        s_id2node = vector<QualifyPointer>();
        s_nodeMap = map<int, int>();
        current_layer = 1;
        Candidates = 0;
        onelayer = a_onelayer;
    };
    ~searchQueue(){};
    //
    // update
    void addWatchtower(QualifyPointer QP)
    {
        if(onelayer == false)
        {
            if(QP->level<current_layer)
            {
                delete QP;
                return ;
            }
        }

        int object = QP->object_ID;
        map<int,int>::iterator iter = s_nodeMap.find(object);
        if ( iter == s_nodeMap.end() ) {
            int end = s_id2node.size()-1;
            s_id2node.push_back(QP);
            s_nodeMap[object] = end+1;
        }
        else {
            int xiabiao = iter->second;
            if(updateWatchtower(QP, s_id2node[xiabiao]))
            {
                QualifyPointer del= s_id2node[xiabiao];
                delete del;
                s_id2node[xiabiao] = QP;
            }
        }
    };


    bool updateWatchtower(QualifyPointer QP, QualifyPointer hashQP)
    {
        int object = QP->object_ID;
        float exist = hashQP->farFromObject + hashQP->current_cost;
        float insert = QP->farFromObject + QP->current_cost;
        if(exist <= insert)
            return false;
        else
            return true;
    };

    int checkBound()
    {
        int count = 0;
        for(int i=0; i<s_id2node.size(); i++)
        {
            if(s_id2node[i]->level<= current_layer)
                count ++;
        }
        return count;
    };


    void setLayer(int a_layer)
    {
        current_layer = a_layer;
    };


    void clean()
    {
        for(int i=0; i<s_id2node.size(); i++)
        {
            delete s_id2node[i];
        }
    };

    void print(int KNN)
    {
        for(int i=0; i<KNN; i++)
        {
            cout<<"distance = "<< s_id2node[i]->current_cost+s_id2node[i]->farFromObject<<endl;
        }
    };

    void getKNN(int kNN, const bool a_print = false)
    {
        sort(s_id2node.begin(), s_id2node.end(), compareByLength);
        if(a_print == true)
            print(kNN);
    };


};

#endif


