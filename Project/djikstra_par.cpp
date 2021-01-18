#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <climits>
#include <chrono>
#include <string>
#include <thread>
#include <unordered_set>
//new headers required by the parallel version
#include <boost/mpi.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <omp.h>

const int THREAD_COUNT = 4;
const int SPLITTER_ID = 0;

#define FREE_WORKER_TAG 1
#define ASSIGN_WORK_TAG 2
#define WORKER_TO_COLLECTOR_TAG 3


class Edge{
    public:
        //unique identifier of node that owns this edge
        char parentNode[21] = "00000000000000000000";
        //boolean true: edge part of in vector, false: edge part of out vector
        bool isIn=false;
        //weight of the edge
        unsigned long long int weight = ULLONG_MAX;
        //unique identifier of other node
        char otherNode[21] = "00000000000000000000";
        //distance from starting node
        unsigned long long distance = ULLONG_MAX;
        //workaround, not proper way to do that
        void send(boost::mpi::communicator world, const int dest, const int tag) const{
            world.send(dest, tag+2, parentNode);
            world.send(dest, tag+3, isIn);
            world.send(dest, tag+4, weight);
            world.send(dest, tag+5, otherNode);
            world.send(dest, tag+6, distance);
        }
        static Edge irecv(boost::mpi::communicator world, int src, const int tag){
            Edge e;
            boost::optional<boost::mpi::status> stat;
            boost::mpi::request req=world.irecv(src, tag+2, e.parentNode);
            for(int i=0;i<30;++i){
                stat = req.test();
                if(stat){
                    world.recv(stat->source(), tag+3, e.isIn);
                    world.recv(stat->source(), tag+4, e.weight);
                    world.recv(stat->source(), tag+5, e.otherNode);
                    world.recv(stat->source(), tag+6, e.distance);
                    return e;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            strcpy(e.parentNode, "");
            strcpy(e.otherNode, "");
            return e;
        }
        bool operator==(const Edge &other) const{
            return parentNode == other.parentNode && otherNode==other.otherNode;
        }
};
struct EdgeHash{
    std::size_t operator()(const Edge& e) const{
        return ((std::hash<std::string>()(e.parentNode) ^ (std::hash<std::string>()(e.otherNode) << 1)) >> 1);
    }
};
class Node{
    public:
        std::unordered_set<Edge, EdgeHash> in = std::unordered_set<Edge, EdgeHash>();
        std::unordered_set<Edge, EdgeHash> out = std::unordered_set<Edge, EdgeHash>();
        std::string name;   //unique identifier of a Node
        unsigned long long int distance = ULLONG_MAX; //distance from start
        std::string previousNode; //unique identifier of previous node nearest to start
};

void djikstra_emitter(std::vector<Node> &start, boost::mpi::communicator world){
    std::queue<Node> queue;
    queue.push(start[0]);  //start enters the queue
    int free_worker_id;
    while(!queue.empty()){
        world.recv(boost::mpi::any_source, FREE_WORKER_TAG, free_worker_id);
        Node current = queue.front();    //get first node
        queue.pop(); //removes first node
        world.send(free_worker_id, ASSIGN_WORK_TAG, current.name);
        if(queue.empty()){
            Edge e;
            e = Edge::irecv(world, boost::mpi::any_source, WORKER_TO_COLLECTOR_TAG);
            if(strcmp(e.parentNode,"")==0 || strcmp(e.otherNode, "")==0){
                break;
            }
            for(auto & i : start){
                if(strcmp(i.name.c_str(),e.otherNode)==0){
                    i.distance=e.distance;
                    i.previousNode = e.parentNode;
                    queue.push(i);
                }
            }
        }
    }
    std::string exit_msg = "exit";
    for(int worker_id = SPLITTER_ID+1; worker_id<THREAD_COUNT;++worker_id){
        world.send(worker_id, ASSIGN_WORK_TAG, exit_msg);
    }
}

void djikstra_worker(std::vector<Node> &graph, boost::mpi::communicator world){
    while(true){
        //notify that worker is free
        world.send(SPLITTER_ID, FREE_WORKER_TAG, world.rank());
        std::string node_name;
        world.recv(SPLITTER_ID, ASSIGN_WORK_TAG, node_name);
        //might happen only in the end
        if(node_name=="exit"){
            return;
        }
        Node n;
        #pragma omp parallel num_threads(THREAD_COUNT/2) shared(world)
        {
            #pragma omp for
            for (auto & tmp : graph) {
                if (tmp.name == node_name) {
                    n = tmp;
                }
            }
        }
        #pragma omp parallel for num_threads(THREAD_COUNT/2)
        for(auto it = n.out.begin(); it != n.out.end(); ++it){    //for all neighbor of the node
            unsigned long long int tmpDistance = n.distance+(*it).weight;
            if(tmpDistance<(*it).distance){
                Edge tmpOut;
                tmpOut.distance = tmpDistance;
                tmpOut.isIn = (*it).isIn;
                strcpy(tmpOut.parentNode, (*it).parentNode);
                strcpy(tmpOut.otherNode, (*it).otherNode);
                tmpOut.weight = (*it).weight;
                tmpOut.send(world, SPLITTER_ID, WORKER_TO_COLLECTOR_TAG);
            }
        }
    }
}


Node generate_node(std::string name){
    Node n;
    n.name = std::move(name);
    return n;
}

void make_graph(std::vector<Node> &list){
    for(long unsigned int i = 0;i<list.size();++i){
        //make edges
        for(long unsigned int j = 0;j<rand()%list.size()+1;++j){
            Edge e;
            e.weight=rand()%list.size();
            long unsigned int pos = rand()%list.size();
            if(pos!=i){
                strcpy(e.parentNode, list[pos].name.c_str());
                e.isIn = true;
                strcpy(e.otherNode, list[i].name.c_str());
                e.distance = list[i].distance;
                list[pos].in.insert(e);
                
                strcpy(e.parentNode, list[i].name.c_str());
                e.isIn = false;
                strcpy(e.otherNode, list[pos].name.c_str());
                e.distance = list[pos].distance;
                list[i].out.insert(e);
            }
        }
    }
}

std::string generate_name(){
    std::string alphabeth = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
    std::string out;
    for(int i = 0;i<20;++i){
        out += alphabeth[rand()%alphabeth.length()];
    }
    return out;
}

int main(int argc, char* argv[]){
    if(argc<=1){
        std::cout<<"Usage: "<<argv[0]<<" [number of nodes]\n";
        return 0;
    }

    std::vector<Node> graph;
    boost::mpi::environment env; //initialize the environment
    boost::mpi::communicator world; //initialize communicator
    
    srand(123456);
    for(int i = 0;i<atoi(argv[1]);++i){
        graph.push_back(generate_node(generate_name()));
    }

    make_graph(graph);

    graph[0].distance=0;
    // to have at least one edge
    Edge tmp;
    strcpy(tmp.parentNode, graph[0].name.c_str());
    tmp.isIn = false;
    tmp.weight = rand()%graph.size();
    strcpy(tmp.otherNode, graph[graph.size()-1].name.c_str());
    tmp.distance = graph[graph.size()-1].distance;
    graph[0].out.insert(tmp);
    //letting know last that has a edge coming from first
    strcpy(tmp.parentNode, graph[graph.size()-1].name.c_str());
    tmp.isIn = true;
    strcpy(tmp.otherNode, graph[0].name.c_str());
    tmp.distance = graph[0].distance;
    graph[graph.size()-1].in.insert(tmp);
    //-------------------------------

    if(world.rank()==SPLITTER_ID){
        auto funct_start = std::chrono::steady_clock::now();
        djikstra_emitter(graph, world);
        auto funct_end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start);
        std::cout<<"Time required by djikstra_par: "<<duration.count()<<" milliseconds\n";
        for(long unsigned int i = 0 ;i<graph.size();++i){
            std::cout<<"node "<<i<<"\n"
                <<graph[i].name<<" distance:"<<graph[i].distance<<"\n"
                <<"\n--------------------------------------------------\n"
                <<"out nodes: \n";
            for(auto & j : graph[i].out){
                std::cout<<"\t\t"<<j.weight<<" "<<j.otherNode<<"\n";
            }
            std::cout<<"\n--------------------------------------------------\n";
            std::cout<<"in nodes: \n";
            for(auto & j : graph[i].in){
                std::cout<<"\t\t"<<j.weight<<" "<<j.otherNode<<"\n";
            }
            std::cout<<"\n--------------------------------------------------\n";
        }
    }else{
        djikstra_worker(world);
    }
    return 0;
}
