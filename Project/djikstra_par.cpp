#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <chrono>
//-------------------
//new headers required by the parallel version
#include <boost/mpi.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <omp.h>

const int THREAD_COUNT = 8;
const int COLLECTOR_ID = THREAD_COUNT-1;
const int SPLITTER_ID = 0;
const int NUM_OF_WORKERS = THREAD_COUNT-2;

#define FREE_WORKER_TAG 1
#define ASSIGN_WORK_TAG 2
#define WORKER_TO_COLLECTOR_TAG 3
#define COLLECTOR_TO_SPLITTER_TAG 4

class Edge;

class Node{
    public:
        std::vector<Edge> in, out; 
        std::string name = "";   //unique identifier of a Node
        unsigned long long int distance = ULLONG_MAX; //distance from start
        std::string previousNode = ""; //unique identifier of previous node nearest to start 
        
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            std::cout<<"(de/)serialization start Node\n";
            ar & in;
            //std::cout<<"in complete\n";
            ar & out;
            //std::cout<<"out complete\n";
            ar & name;
            //std::cout<<"name complete\n";
            ar & distance;
            //std::cout<<"distance complete\n";
            ar & previousNode;
            std::cout<<"node (de/)serialization complete\n";
        }
};

class Edge{
    public:
        //unique identifier of node that owns this edge
        std::string parentNode="";
        //boolean true: edge part of in vector, false: edge part of out vector
        bool isIn=false;
        //weight of the edge
        long long int weight = LLONG_MAX;
        //unique identifier of other node
        std::string otherNode = "";
        //distance from starting node
        unsigned long long distance = ULLONG_MAX;
        
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, const unsigned int version){
            std::cout<<"edge (de/)serialization start\n";
            ar & weight;
            std::cout<<"weight complete\t";
            ar & distance;
            std::cout<<"distance complete\t";
            ar & isIn;
            std::cout<<"isIn complete\t";
            ar & parentNode;
            std::cout<<"parent node complete\t";
            ar & otherNode;
            std::cout<<"other node complete\n";
            std::cout<<"edge (de/)serialization complete\n";
        }
};

void djikstra_splitter(std::vector<Node> &start, boost::mpi::communicator world){
    std::queue<Node> queue;
    queue.push(start[0]);  //start enters the queue
    int free_worker_id;
    while(!queue.empty()){
        world.recv(boost::mpi::any_source, FREE_WORKER_TAG, free_worker_id);
        Node current = queue.front();    //get first node
        queue.pop(); //removes first node
        //current.fix_before_serializing();
        std::cout<<"assigning work to "<<free_worker_id<<"\n";
        world.send(free_worker_id, ASSIGN_WORK_TAG, &current);
        std::cout<<"in2\n";
        if(queue.empty()){
            std::cout<<"empty queue 1\n";
            Edge e;
            //receive from collector edge to add in the queue
            //blocking call (may create problems in the end)
            world.recv(COLLECTOR_ID, COLLECTOR_TO_SPLITTER_TAG, e);
            std::cout<<"empty queue 2\n";
            //impossible
            if(e.otherNode == "" || e.parentNode==""){
                std::cout<<"exit 1\n";
                break;
            }
            bool done1 = false, done2 = false;
            for(long unsigned int i = 0; i<start.size() && !(done1 && done2);++i){
                if(start[i].name==e.otherNode){
                    queue.push(start[i]);
                    done1=true;
                }else if(start[i].name==e.parentNode){
                    if(e.isIn){
                        for(long unsigned int j=0;j<start[i].in.size();++j){
                            if(start[i].in[i].otherNode==e.otherNode){
                                start[i].in[i].distance = e.distance;
                                break;
                            }
                        }
                    }else{
                        for(long unsigned int j=0;j<start[i].out.size();++j){
                            if(start[i].out[i].otherNode==e.otherNode){
                                start[i].out[i].distance = e.distance;
                                break;
                            }
                        }
                    }
                    done2=true;
                }
            }
            if(!done1){
                std::cout<<"Not pushed new node\n";
            }
            if(!done2){
                std::cout<<"Not updated edge\n";
            }
        }
    }
    std::cout<<"exit2\n";
    Node final;
    final.name = "";
    for(int worker_id = SPLITTER_ID+1; worker_id<COLLECTOR_ID;++worker_id){
        //final.fix_before_serializing();
        world.isend(worker_id, ASSIGN_WORK_TAG, final);
    }
}
void djikstra_collector(boost::mpi::communicator world){
    Edge e;
    while(true){
        world.recv(boost::mpi::any_source, WORKER_TO_COLLECTOR_TAG, e);
        //n.fix_before_serializing();
        world.isend(SPLITTER_ID, COLLECTOR_TO_SPLITTER_TAG, &e);
        if(e.otherNode == "" || e.parentNode == ""){
            break;
        }
    }
}
void djikstra_worker(boost::mpi::communicator world){
    while(true){
        //notify with potentially blocking call that worker is free
        world.send(SPLITTER_ID, FREE_WORKER_TAG, world.rank());
        std::cout<<"waiting for work rank "<<world.rank()<<"\n";
        Node n;
        world.recv(SPLITTER_ID, ASSIGN_WORK_TAG, n);
        std::cout<<"got work rank "<<world.rank()<<"\n";
        //might happen only in the end
        if(n.name==""){
            if(world.rank()==COLLECTOR_ID-1){
                //n.fix_before_serializing();
                world.send(COLLECTOR_ID, WORKER_TO_COLLECTOR_TAG, &n);
            }
            break;
        }
        //use open mp to parallelize this for cicle
        for(long unsigned int i = 0;i<n.out.size();++i){    //for all neighbor of the node
            unsigned long long int tmpDistance = n.distance+n.out[i].weight;
            if(tmpDistance<n.out[i].distance){
                n.out[i].distance=tmpDistance;
                n.out[i].otherNode=n.name;
                //non blocking call to send new node in queue
                //n.out[i].n->fix_before_serializing();
                world.isend(COLLECTOR_ID, WORKER_TO_COLLECTOR_TAG, n.out[i]);
            }
        }
    }
}


Node generate_node(std::string name){
    Node n;
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
                e.parentNode = list[pos].name;
                e.isIn = true;
                e.otherNode = list[i].name;
                e.distance = list[i].distance;
                list[pos].in.push_back(e);
                
                e.parentNode = list[i].name;
                e.isIn = false;
                e.otherNode = list[pos].name;
                e.distance = list[pos].distance;
                list[i].out.push_back(e);
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
    tmp.parentNode = graph[0].name;
    tmp.isIn = false;
    tmp.weight = rand()/2;
    tmp.otherNode = graph[graph.size()-1].name;
    tmp.distance = graph[graph.size()-1].distance;
    graph[0].out.push_back(tmp);
    //letting know last that has a edge coming from first
    tmp.parentNode = graph[graph.size()-1].name;
    tmp.isIn = true;
    tmp.otherNode = graph[0].name;
    tmp.distance = graph[0].distance;
    graph[graph.size()-1].in.push_back(tmp);
    //-------------------------------

    if(world.rank()==SPLITTER_ID){
        auto funct_start = std::chrono::steady_clock::now();
        djikstra_splitter(graph, world);
        auto funct_end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start);
        std::cout<<"Time required by djikstra_par: "<<duration.count()<<" milliseconds\n";
        for(long unsigned int i = 0 ;i<graph.size();++i){
            std::cout<<"node "<<i<<"\n"
                <<graph[i].name<<" distance:"<<graph[i].distance<<"\n"
                <<"\n--------------------------------------------------\n"
                <<"out nodes: \n";
            for(long unsigned int j = 0;j<graph[i].out.size();++j){
                std::cout<<"\t\t"<<graph[i].out[j].weight<<" "<<graph[i].out[j].otherNode<<"\n";
            }
            std::cout<<"\n--------------------------------------------------\n";
            std::cout<<"in nodes: \n";
            for(long unsigned int j = 0;j<graph[i].in.size();++j){
                std::cout<<"\t\t"<<graph[i].in[j].weight<<" "<<graph[i].in[j].otherNode<<"\n";
            }
            std::cout<<"\n--------------------------------------------------\n";
        }   
    }else if(world.rank()==COLLECTOR_ID){
        djikstra_collector(world);
    }else{
        djikstra_worker(world);
    }
    return 0;
}