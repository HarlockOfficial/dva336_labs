#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <chrono>
//-------------------
//new headers required by the parallel version
#include <omp.h>
#include <boost/mpi.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

const int THREAD_COUNT = 8;
const int COLLECTOR_ID = THREAD_COUNT-1;
const int SPLITTER_ID = 0;
const int NUM_OF_WORKERS = THREAD_COUNT-2;

#define FREE_WORKER_TAG 1
#define ASSIGN_WORK_TAG 2
#define WORKER_TO_COLLECTOR_TAG 3
#define COLLECTOR_TO_SPLITTER_TAG 4

class Node;

class Edge{
    public:
        long long int weight = LLONG_MAX;
        Node n[1] = nullptr;
    private:
        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, const unsigned int version){
            ar & weight;
            ar & n;
        }
};

class Node{
    public:
        std::vector<Edge> in, out; 
        std::string name = "";   //unique identifier of a Node
        unsigned long long int distance = ULLONG_MAX; //default infinity
        Node nearestPrev[1] = nullptr;   //default NULL
    private:
        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, const unsigned int version){
            ar & in;
            ar & out;
            ar & name;
            ar & distance;
            ar & nearestPrev;
        }
};

void djikstra_splitter(std::vector<Node> &start,
            boost::mpi::communicator world){
    std::queue<Node> queue;
    queue.push(start[0]);  //start enters the queue
    int free_worker_id;
    while(!queue.empty()){
        world.recv(boost::mpi::any_source, FREE_WORKER_TAG, free_worker_id);
        Node current = queue.front();    //get first node
        queue.pop(); //removes first node
        world.send(free_worker_id, ASSIGN_WORK_TAG, &current);
        
        if(queue.empty()){
            Node n;
            //receive from collector queue containing other nodes
            //blocking call (may create problems in the end)
            world.recv(COLLECTOR_ID, COLLECTOR_TO_SPLITTER_TAG, n);
            //if n not null
            //impossible
            if(n.name == ""){
                break;
            }
            queue.push(*n);
        }
    }
    Node final;
    final.name = "";
    for(int worker_id = SPLITTER_ID+1; worker_id<COLLECTOR_ID;++worker_id){
        world.send(worker_id, ASSIGN_WORK_TAG, final);
    }
}
void djikstra_collector(boost::mpi::communicator world){
    Node n;
    while(true){
        world.recv(boost::mpi::any_source, WORKER_TO_COLLECTOR_TAG, &n);
        world.isend(SPLITTER_ID, COLLECTOR_TO_SPLITTER_TAG, &n);
        if(n.name == ""){
            break;
        }
    }
}
void djikstra_worker(boost::mpi::communicator world){
    Node n;
    while(true){
        //notify with blocking call that worker is free
        world.send(SPLITTER_ID, FREE_WORKER_TAG, world.rank());
        world.recv(SPLITTER_ID, ASSIGN_WORK_TAG, &n);
        //never happens, always receive front of queue
        if(n.name==""){
            if(world.rank()==COLLECTOR_ID-1){
                world.send(COLLECTOR_ID, WORKER_TO_COLLECTOR_TAG, &n);
            }
            break;
        }
        //use open mp to parallelize this for cicle
        for(long unsigned int i = 0;i<n.out.size();++i){    //for all neighbor of the node
            unsigned long long int tmpDistance = n.distance+n.out[i].weight;
            if(tmpDistance<n.out[i].n->distance){
                n.out[i].n->distance=tmpDistance;
                n.out[i].n->nearestPrev=&n;
                //non blocking call to send new node in queue
                world.isend(COLLECTOR_ID, WORKER_TO_COLLECTOR_TAG, &(n.out[i].n));
            }
        }
    }
}

Node generate_node(std::string name){
    Node n;
    n.distance=LLONG_MAX;
    n.nearestPrev = NULL;
    n.name = name;
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
                e.n = &(list[pos]);
                list[i].in.push_back(e);
                e.n = &(list[i]);
                list[pos].out.push_back(e);
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
    boost::mpi::environment env(argc, argv); //initialize the environment
    boost::mpi::communicator world; //initialize communicator
    
    srand(123456);
    for(int i = 0;i<atoi(argv[1]);++i){
        graph.push_back(generate_node(generate_name()));
    }

    make_graph(graph);

    graph[0].distance=0;
    graph[0].nearestPrev=NULL;
    // to have at least one edge
    Edge tmp;
    tmp.weight = rand()/2;
    //create edge exiting from first
    tmp.n = &(graph[graph.size()-1]);
    graph[0].out.push_back(tmp);
    //letting know last that has a edge coming from first
    tmp.n = &(graph[0]);
    graph[graph.size()-1].in.push_back(tmp);
    //-------------------------------

    if(world.rank()==SPLITTER_ID){
        djikstra_splitter(graph, world);
    }else if(world.rank()==COLLECTOR_ID){
        djikstra_collector(world);
    }else{
        djikstra_worker(world);
    }

    auto funct_start = std::chrono::steady_clock::now();
    auto funct_end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start);
    std::cout<<"Time required by djikstra_par: "<<duration.count()<<" milliseconds\n";
    for(long unsigned int i = 0 ;i<graph.size();++i){
        std::cout<<"node "<<i<<"\n"
            <<graph[i].name<<" distance:"<<graph[i].distance<<"\n"
            <<"\n--------------------------------------------------\n"
            <<"out nodes: \n";
        for(long unsigned int j = 0;j<graph[i].out.size();++j){
            std::cout<<"\t\t"<<graph[i].out[j].weight<<" "<<graph[i].out[j].n->name<<"\n";
        }
        std::cout<<"\n--------------------------------------------------\n";
        std::cout<<"in nodes: \n";
        for(long unsigned int j = 0;j<graph[i].in.size();++j){
            std::cout<<"\t\t"<<graph[i].in[j].weight<<" "<<graph[i].in[j].n->name<<"\n";
        }
        std::cout<<"\n--------------------------------------------------\n";
    }
    
    return 0;
}