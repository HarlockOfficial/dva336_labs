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

class Edge;

class Node{
    public:
        std::vector<Edge> in, out; 
        std::string name = "";   //unique identifier of a Node
        unsigned long long int distance = ULLONG_MAX; //default infinity
        Node* nearestPrev;   //default NULL
        void fix_before_sending() const {
            if(nearestPrev == NULL){
                return;
            }
            //will give error
            nearestPrev->in = std::vector<Edge>();
            nearestPrev->out= std::vector<Edge>();
            //nearestPrev->fix_before_sending();
            nearestPrev->nearestPrev = nullptr;
            //------------------------------ 
            //std::cout<<"nearestPrev data ready to be serialized\n";
        }
        friend class boost::serialization::access;
        template <class Archive>
        void save(Archive& ar, const unsigned int version) const {
            fix_before_sending();
            //std::cout<<"serialization start Node\n";
            ar & in;
            //std::cout<<"in serialization complete\n";
            ar & out;
            //std::cout<<"out serialization complete\n";
            ar & name;
            //std::cout<<"name serialization complete\n";
            ar & distance;
            //std::cout<<"distance serialization complete\n";
            //the error might be that nearestPrev is changed during deserialization, 
            //and the serialization fails
            if(nearestPrev!=NULL){
                nearestPrev->serialize(ar, version);
            }else{
                ar & nearestPrev;
            }
            //std::cout<<"nearestPrev serialization complete\n";
        }
        template<class Archive>
        void load(Archive & ar, const unsigned int version){
            ar & in;
            ar & out;
            ar & name;
            ar & distance;
            ar & nearestPrev;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
};

class Edge{
    public:
        long long int weight = LLONG_MAX;
        Node* n;
        void fix_before_sending() const {
            if(n == NULL){ //impossible
                return;
            }
            //will give error
            n->in = std::vector<Edge>();
            n->out= std::vector<Edge>();
            n->nearestPrev = nullptr;
            //------------------------------
            //std::cout<<"n data ready to be serialized\n";
        }
        friend class boost::serialization::access;
        template <class Archive>
        void save(Archive& ar, const unsigned int version) const {
            fix_before_sending();
            //std::cout<<"serialization start Edge\n";
            ar & weight;
            //std::cout<<"serialized weight";
            if(n != NULL){
                n->serialize(ar, version);
            }else{
                ar & n;
            }
            //std::cout<<"serialization complete Edge\n";
        }
        template <class Archive>
        void load(Archive& ar, const unsigned int version){
            ar & weight;
            ar & n;
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
};

void djikstra_splitter(std::vector<Node> &start, boost::mpi::communicator world){
    std::queue<Node> queue;
    queue.push(start[0]);  //start enters the queue
    int free_worker_id;
    while(!queue.empty()){
        world.recv(boost::mpi::any_source, FREE_WORKER_TAG, free_worker_id);
        Node current = queue.front();    //get first node
        queue.pop(); //removes first node

        //should solve serialization missing data
        for(Node n: start){
            if(current.name==n.name){
                n.distance = current.distance;
                current = n;
                break;
            }
        }
        // maybe
        //current.fix_before_sending();
        std::cout<<"assigning work to "<<free_worker_id<<"\n";
        world.send(free_worker_id, ASSIGN_WORK_TAG, &current);
        std::cout<<"in2\n";
        if(queue.empty()){
            std::cout<<"empty queue 1\n";
            Node n;
            //receive from collector queue containing other nodes
            //blocking call (may create problems in the end)
            world.recv(COLLECTOR_ID, COLLECTOR_TO_SPLITTER_TAG, n);
            std::cout<<"empty queue 2\n";
            //if n not null
            //impossible
            if(n.name == ""){
                std::cout<<"exit 1\n";
                break;
            }
            //should solve serialization missing data
            for(Node tmp: start){
                if(n.name==tmp.name){
                    tmp.distance = n.distance;
                    n = tmp;
                    break;
                }
            }
            // maybe
            queue.push(n);
        }
    }
    std::cout<<"exit2\n";
    Node final;
    final.name = "";
    for(int worker_id = SPLITTER_ID+1; worker_id<COLLECTOR_ID;++worker_id){
        //final.fix_before_sending();
        world.isend(worker_id, ASSIGN_WORK_TAG, final);
    }
}
void djikstra_collector(boost::mpi::communicator world){
    Node n;
    while(true){
        world.recv(boost::mpi::any_source, WORKER_TO_COLLECTOR_TAG, n);
        //n.fix_before_sending();
        world.isend(SPLITTER_ID, COLLECTOR_TO_SPLITTER_TAG, &n);
        if(n.name == ""){
            break;
        }
    }
}
void djikstra_worker(boost::mpi::communicator world){
    Node n;
    while(true){
        //notify with potentially blocking call that worker is free
        world.send(SPLITTER_ID, FREE_WORKER_TAG, world.rank());
        std::cout<<"waiting for work rank "<<world.rank()<<"\n";
        //TODO fix deserialization problem length error, may be vector
        world.recv(SPLITTER_ID, ASSIGN_WORK_TAG, n);
        std::cout<<"got work rank "<<world.rank()<<"\n";
        //never happens, always receive front of queue
        if(n.name==""){
            if(world.rank()==COLLECTOR_ID-1){
                //n.fix_before_sending();
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
                //TODO fix nearestPrev might be reset
                //non blocking call to send new node in queue
                //n.out[i].n->fix_before_sending();
                world.isend(COLLECTOR_ID, WORKER_TO_COLLECTOR_TAG, n.out[i].n);
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
    boost::mpi::environment env; //initialize the environment
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
                std::cout<<"\t\t"<<graph[i].out[j].weight<<" "<<graph[i].out[j].n->name<<"\n";
            }
            std::cout<<"\n--------------------------------------------------\n";
            std::cout<<"in nodes: \n";
            for(long unsigned int j = 0;j<graph[i].in.size();++j){
                std::cout<<"\t\t"<<graph[i].in[j].weight<<" "<<graph[i].in[j].n->name<<"\n";
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