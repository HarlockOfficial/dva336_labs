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

#define THREAD_COUNT 4
#define SPLITTER_ID 0

#define FREE_WORKER_TAG 1
#define ASSIGN_WORK_TAG 2
#define WORKER_TO_COLLECTOR_TAG 3


class Edge{
    public:
        //unique identifier of node that owns this edge
        char parentNode[21];
        //weight of the edge
        unsigned long long int weight;
        //unique identifier of other node
        char otherNode[21];
        int edge_src;
        //workaround, not proper way to do that
        void send(boost::mpi::communicator world, const int dest, const int tag) const{
            world.send(dest, tag+3, parentNode);
            world.send(dest, tag+4, weight);
            world.send(dest, tag+5, otherNode);
        }
        static Edge irecv(boost::mpi::communicator world, int src, const int tag){
            Edge e{};
            boost::optional<boost::mpi::status> stat;
            boost::mpi::request req=world.irecv(src, tag+3, e.parentNode);
            for(int i=0;i<1000;++i){
                stat = req.test();
                if(stat){
                    world.recv(stat->source(), tag+4, e.weight);
                    world.recv(stat->source(), tag+5, e.otherNode);
                    e.edge_src = stat->source();
                    return e;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            char exit_empty[21] = "exit_empty";
            strncpy(e.parentNode, exit_empty, 21);
            strncpy(e.otherNode, exit_empty, 21);
            return e;
        }
        static Edge recv(boost::mpi::communicator world, int src, const int tag){
            Edge e{};
            world.recv(src, tag+3, e.parentNode);
            world.recv(src, tag+4, e.weight);
            world.recv(src, tag+5, e.otherNode);
            return e;
        }
};

class Node{
    public:
        std::vector<Edge> out = std::vector<Edge>();
        std::string name;   //unique identifier of a Node
        unsigned long long int distance{}; // from start
};

void dijkstra_emitter(std::vector<Node> &start, boost::mpi::communicator world){
    std::queue<Node> queue;
    queue.push(start[0]);  //start enters the queue
    while(!queue.empty()){
        Node current = queue.front();    //get first node
        queue.pop(); //removes first node
        unsigned long long int current_distance = current.distance;
        #pragma omp parallel for
        for(int i=0;i<current.out.size();++i){ // NOLINT(modernize-loop-convert)
            int free_worker_id;
            unsigned long long int destination_distance;
            for (const Node &tmp: start) {
                if (strncmp(current.out[i].otherNode, tmp.name.c_str(), 21) == 0) {
                    destination_distance = tmp.distance;
                    break;
                }
            }
            #pragma omp critical
            {
                world.recv(boost::mpi::any_source, FREE_WORKER_TAG, free_worker_id);
                current.out[i].send(world, free_worker_id, ASSIGN_WORK_TAG);
                world.send(free_worker_id, ASSIGN_WORK_TAG, current_distance);
                world.send(free_worker_id, ASSIGN_WORK_TAG + 1, destination_distance);
            }
        }
        if(queue.empty()){
            Edge e = Edge::irecv(world, boost::mpi::any_source, WORKER_TO_COLLECTOR_TAG);
            if(strcmp(e.parentNode,"exit_empty")==0 || strcmp(e.otherNode, "exit_empty")==0){
                break;
            }
            unsigned long long int current_destination_node_distance;
            world.recv(e.edge_src, WORKER_TO_COLLECTOR_TAG+1, current_destination_node_distance);
            for(auto & i : start){
                if(strcmp(i.name.c_str(),e.otherNode)==0){
                    i.distance = current_destination_node_distance;
                    queue.push(i);
                    break;
                }
            }
        }
    }
    Edge e{};
    strncpy(e.parentNode, "exit", 21);
    strncpy(e.otherNode, "exit", 21);
    for(int worker_id = SPLITTER_ID+1; worker_id<THREAD_COUNT;++worker_id){
        e.send(world, worker_id, ASSIGN_WORK_TAG);
    }
}

void dijkstra_worker(const std::vector<Node>& graph, boost::mpi::communicator world){
    while(true){
        //notify that worker is free
        world.send(SPLITTER_ID, FREE_WORKER_TAG, world.rank());
        Edge e = Edge::recv(world, SPLITTER_ID, ASSIGN_WORK_TAG);
        //will happen only in the end
        if(strcmp(e.parentNode, "exit")==0){
            return;
        }
        unsigned long long int node_distance;
        unsigned long long int destination_node_distance;
        world.recv(SPLITTER_ID, ASSIGN_WORK_TAG, node_distance);
        world.recv(SPLITTER_ID, ASSIGN_WORK_TAG+1, destination_node_distance);
        unsigned long long int tmpDistance = node_distance+e.weight;
        if(tmpDistance<destination_node_distance){
            Edge tmpOut{};
            destination_node_distance = tmpDistance;
            strcpy(tmpOut.parentNode, e.parentNode);
            strcpy(tmpOut.otherNode, e.otherNode);
            tmpOut.weight = e.weight;
            tmpOut.send(world, SPLITTER_ID, WORKER_TO_COLLECTOR_TAG);
            world.send(SPLITTER_ID, WORKER_TO_COLLECTOR_TAG+1, destination_node_distance);
        }
    }
}


Node generate_node(std::string name){
    Node n;
    n.distance = ULLONG_MAX;
    n.name = std::move(name);
    return n;
}

void make_graph(std::vector<Node> &list){
    for(long unsigned int i = 0;i<list.size();++i){
        //make edges
        for(long unsigned int j = 0;j<rand()%list.size()+1;++j){
            Edge e{};
            e.weight=rand()%list.size();
            long unsigned int pos = rand()%list.size();
            if(pos!=i){
                strcpy(e.parentNode, list[i].name.c_str());
                strcpy(e.otherNode, list[pos].name.c_str());
                list[i].out.push_back(e);
            }
        }
    }
}

std::string generate_name(){
    std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
    std::string out;
    for(int i = 0;i<20;++i){
        out += alphabet[rand()%alphabet.length()];
    }
    return out;
}

int main(int argc, char* argv[]){
    std::vector<Node> graph;
    boost::mpi::environment env; //initialize the environment
    boost::mpi::communicator world; //initialize communicator


    if(argc<=1){
        if(world.rank()==SPLITTER_ID){
            std::cout<<"Usage: "<<argv[0]<<" [number of nodes]\n";
        }
        return 0;
    }

    if(world.rank()==SPLITTER_ID){
        srand(123456);
        for(int i = 0;i<atoi(argv[1]);++i){
            graph.push_back(generate_node(generate_name()));
        }

        make_graph(graph);
        graph[0].distance = 0;
        if(graph[0].out.size()==0) {
            // start must have at least one edge
            Edge tmp{};
            strcpy(tmp.parentNode, graph[0].name.c_str());
            tmp.weight = rand() % graph.size();
            strcpy(tmp.otherNode, graph[graph.size() - 1].name.c_str());
            graph[0].out.push_back(tmp);
        }
        auto funct_start = std::chrono::steady_clock::now();
        dijkstra_emitter(graph, world);
        auto funct_end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start);
        std::cout<<"Time required by dijkstra_par: "<<duration.count()<<" milliseconds\n";
        for(long unsigned int i = 0 ;i<graph.size();++i){
            std::cout<<"node "<<i<<"\n"
                <<graph[i].name<<" distance:"<<graph[i].distance<<"\n"
                <<"\n--------------------------------------------------\n"
                <<"out edges: \n";
            for(auto & j : graph[i].out){
                std::cout<<"\t\t"<<j.weight<<" "<<j.otherNode<<"\n";
            }
            std::cout<<"\n--------------------------------------------------\n";
        }
    }else{
        dijkstra_worker(graph, world);
    }
    return 0;
}
