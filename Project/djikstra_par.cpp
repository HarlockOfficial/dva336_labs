#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <chrono>
#include <string>
#include <thread>
//not strictly necessary, already included by other headers
#include <utility>
//-------------------
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
        int lastSrc = -1;
        void send(boost::mpi::communicator world, const int dest, const int tag){
            world.send(dest, tag+2, parentNode);
            world.send(dest, tag+3, isIn);
            world.send(dest, tag+4, weight);
            world.send(dest, tag+5, otherNode);
            world.send(dest, tag+6, distance);
        }
        void isend(boost::mpi::communicator world, const int dest, const int tag){
            world.isend(dest, tag+2, parentNode);
            world.isend(dest, tag+3, isIn);
            world.isend(dest, tag+4, weight);
            world.isend(dest, tag+5, otherNode);
            world.isend(dest, tag+6, distance);
        }
        Edge recv(boost::mpi::communicator world, const int src, const int tag){
            Edge e;
            boost::mpi::status stat;
            stat=world.recv(src, tag+2, e.parentNode);
            world.recv(stat.source(), tag+3, e.isIn);
            world.recv(stat.source(), tag+4, e.weight);
            world.recv(stat.source(), tag+5, e.otherNode);
            world.recv(stat.source(), tag+6, e.distance);
            e.lastSrc = stat.source();
            return e;
        }
        Edge irecv(boost::mpi::communicator world, const int src, const int tag){
            Edge e;
            boost::optional<boost::mpi::status> stat;
            boost::mpi::request req=world.irecv(src, tag+2, e.parentNode);
            for(int i=0;i<20;++i){
                stat = req.test();
                if(stat){
                    //std::cout<<"start if\n";
                    world.recv(stat->source(), tag+3, e.isIn);
                    //std::cout<<"got inIn\t";
                    world.recv(stat->source(), tag+4, e.weight);
                    //std::cout<<"got weight\t";
                    world.recv(stat->source(), tag+5, e.otherNode);
                    //std::cout<<"got otherNode\t";
                    world.recv(stat->source(), tag+6, e.distance);
                    //std::cout<<"got distance\t";
                    e.lastSrc = stat->source();
                    //std::cout<<"got all\n";
                    return e;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            strcpy(e.parentNode, "");
            strcpy(e.otherNode, "");
            e.lastSrc = -1;
            return e;
        }
        /*
        //not working
        friend class boost::serialization::access;
        template <class Archive>
        void save(Archive& ar, const unsigned int version) const{
            //std::cout<<"edge (de/)serialization start\n";
            ar << weight;
            std::cout<<"weight send "<<weight<<"\n";
            //std::cout<<"weight complete\t";
            ar << distance;
            std::cout<<"distance send "<<distance<<"\n";
            //std::cout<<"distance complete\t";
            ar << isIn;
            std::cout<<"isIn send "<<isIn<<"\n";
            //std::cout<<"isIn complete\t";
            ar << parentNode;
            std::cout<<"parent Node send "<<parentNode<<"\n";
            //std::cout<<"parent node complete\t";
            ar << otherNode;
            std::cout<<"other Node send "<<otherNode<<"\n";
            //std::cout<<"other node complete\n";
            //std::cout<<"edge (de/)serialization complete\n";
        }
        template <class Archive>
        void load(Archive& ar, const unsigned int version){
            //std::cout<<"edge (de/)serialization start\n";
            ar >> weight;
            std::cout<<"weight recv "<<weight<<"\n";
            //std::cout<<"weight complete\t";
            ar >> distance;
            std::cout<<"distance recv "<<distance<<"\n";
            //std::cout<<"distance complete\t";
            ar >> isIn;
            std::cout<<"isin recv "<<isIn<<"\n";
            //std::cout<<"isIn complete\t";
            ar >> parentNode;
            std::cout<<"parent node recv "<<parentNode<<"\n";
            //std::cout<<"parent node complete\t";
            ar >> otherNode;
            std::cout<<"other node recv "<<otherNode<<"\n";
            //std::cout<<"other node complete\n";
            //std::cout<<"edge (de/)serialization complete\n";
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()*/
};

class Node{
    public:
        std::vector<Edge> in = std::vector<Edge>();
        std::vector<Edge> out = std::vector<Edge>(); 
        std::string name = "";   //unique identifier of a Node
        unsigned long long int distance = ULLONG_MAX; //distance from start
        std::string previousNode = ""; //unique identifier of previous node nearest to start 
        //workaround, not proper way to do that
        void send(boost::mpi::communicator world, const int dest, const int tag){
            world.send(dest, tag, name);
            world.send(dest, tag, distance);
            world.send(dest, tag, in.size());
            for(long unsigned int i=0;i<in.size();++i){
                in[i].send(world, dest, tag);
            }
            world.send(dest, tag, out.size());
            for(long unsigned int i=0;i<out.size();++i){
                out[i].send(world, dest, tag);
            }
        }
        void isend(boost::mpi::communicator world, const int dest, const int tag){
            world.isend(dest, tag, name);
            world.isend(dest, tag, distance);
            world.isend(dest, tag, in.size());
            for(long unsigned int i=0;i<in.size();++i){
                in[i].isend(world, dest, tag);
            }
            world.isend(dest, tag, out.size());
            for(long unsigned int i=0;i<out.size();++i){
                out[i].isend(world, dest, tag);
            }
        }
        void recv(boost::mpi::communicator world, const int src, const int tag){
            boost::mpi::status stat;
            stat = world.recv(src, tag, name);
            world.recv(stat.source(), tag, distance);
            long unsigned int lenIn;
            world.recv(stat.source(), tag, lenIn);
            in.resize(lenIn);
            for(int i=0;i<lenIn;++i){
                in[i]=in[i].recv(world, stat.source(), tag);
            }
            long unsigned int lenOut;
            world.recv(stat.source(), tag, lenOut);
            in.resize(lenOut);
            std::vector<Edge> outNew;
            for(int i=0;i<lenOut;++i){
                outNew.push_back(out[i].recv(world, stat.source(), tag));
            }
            out.clear();
            out.assign(outNew.begin(), outNew.end());
        }
        
        /*
        //not working
        friend class boost::serialization::access;
        template <class Archive>
        void save(Archive& ar, const unsigned int version) const{
            //std::cout<<"serialization start Node\n";
            if(in.size()>0){
                ar << in;
            }
            if(out.size()>0){
                ar << out;
            }
            //std::cout<<"send lenOut: "<<out.size()<<"\n";
            //std::cout<<"out complete\n";
            
            //std::cout<<"send lenIn: "<<in.size()<<"\n";
            //std::cout<<"in complete\n";
            ar << name;
            //std::cout<<"name complete\n";
            ar << distance;
            //std::cout<<"distance complete\n";
            ar << previousNode;
            //std::cout<<"node serialization complete\n";
        }
        template <class Archive>
        void load(Archive& ar, const unsigned int version){
            //std::cout<<"deserialization start Node\n";
            if(in.size()>0){
                ar >> in;
            }
            if(out.size()>0){
                ar >> out;
            }
            //std::cout<<"received in lenOut: "<<out.size()<<"\n";
            //std::cout<<"out complete\n";
            //std::cout<<"received in lenIn: "<<in.size()<<"\n";
            //std::cout<<"in complete\n";
            ar >> name;
            //std::cout<<"name complete\n";
            ar >> distance;
            //std::cout<<"distance complete\n";
            ar >> previousNode;
            //std::cout<<"node serialization complete\n";
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()*/
};

bool contains(std::vector<Edge> v, Edge e){
    for(Edge x: v){
        if(strcmp(x.parentNode, e.parentNode)==0 && 
            strcmp(x.otherNode, e.otherNode)==0 && x.weight==e.weight){
                return true;
        }
    }
    return false;
}

void djikstra_splitter(std::vector<Node> &start, boost::mpi::communicator world){
    std::queue<Node> queue;
    queue.push(start[0]);  //start enters the queue
    int free_worker_id;
    std::vector<int> emptyNodeNotified;
    while(!queue.empty()){
        world.recv(boost::mpi::any_source, FREE_WORKER_TAG, free_worker_id);
        Node current = queue.front();    //get first node
        queue.pop(); //removes first node
        //current.fix_before_serializing();
        //std::cout<<"assigning work to "<<free_worker_id<<"\n";
        
        current.send(world, free_worker_id, ASSIGN_WORK_TAG);
        bool done1 = false;
        while(queue.empty()){
            Edge e;
            if(emptyNodeNotified.size()>=(int)(THREAD_COUNT/2)){
                //all workers sent an empty node -> exiting
                break;
            }
            //std::cout<<"Waiting for new Edge to put in queue\n";
            e = e.irecv(world, boost::mpi::any_source, WORKER_TO_COLLECTOR_TAG);
            //std::cout<<e.otherNode<<" "<<e.parentNode<<" "<<e.lastSrc<<"\n";
            if(strcmp(e.otherNode,"")==0 || strcmp(e.parentNode,"")==0){
                if(e.lastSrc == -1){
                    break;
                }
                emptyNodeNotified.push_back(e.lastSrc);
                //std::cout<<e.lastSrc<<" gave empty node\n";
                continue;
            }
            for(long unsigned int i = 0; i<start.size();++i){
                if(strcmp(start[i].name.c_str(),e.otherNode)==0){
                    start[i].distance=e.distance;
                    start[i].previousNode = e.parentNode;
                    queue.push(start[i]);
                    done1=true;
                    break;
                }
            }
            if(!done1){
                std::cout<<"Not pushed new node\n";
                break;
            }
        }
        if(!done1){
            //std::cout<<"exit00\n";
            break;
        }
    }
    Node final;
    final.name = "";
    for(int worker_id = SPLITTER_ID+1; worker_id<THREAD_COUNT;++worker_id){
        //final.fix_before_serializing();
        final.isend(world, worker_id, ASSIGN_WORK_TAG);
    }
}

void djikstra_worker(boost::mpi::communicator world){
    std::vector<Edge> visited;
    while(true){
        //notify that worker is free
        world.isend(SPLITTER_ID, FREE_WORKER_TAG, world.rank());
        Node n;
        n.recv(world, SPLITTER_ID, ASSIGN_WORK_TAG);
        if(n.name==""){
            break;
        }
        bool validNodeFound = false;
        #pragma omp parallel num_threads(THREAD_COUNT/2) shared(world)
        {
            #pragma omp for
            for(long unsigned int i = 0;i<n.out.size();++i){    //for all neighbor of the node
                if(!contains(visited, n.out[i])){   
                    unsigned long long int tmpDistance = n.distance+n.out[i].weight;
                    if(tmpDistance<n.out[i].distance){
                        Edge tmpOut;
                        tmpOut.distance = tmpDistance;
                        tmpOut.isIn = n.out[i].isIn;
                        strcpy(tmpOut.parentNode, n.out[i].parentNode);
                        strcpy(tmpOut.otherNode, n.out[i].otherNode);
                        tmpOut.weight = n.out[i].weight;
                        //non blocking call to send new node in queue
                        tmpOut.isend(world, SPLITTER_ID, WORKER_TO_COLLECTOR_TAG);
                        validNodeFound = true;
                    }
                    visited.push_back(n.out[i]);
                }
            }
        }
        if(!validNodeFound){
            Edge tmpOut;
            strcpy(tmpOut.otherNode, "");
            strcpy(tmpOut.parentNode, "");
            tmpOut.isend(world, SPLITTER_ID, WORKER_TO_COLLECTOR_TAG);
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
                strcpy(e.parentNode, list[pos].name.c_str());
                e.isIn = true;
                strcpy(e.otherNode, list[i].name.c_str());
                e.distance = list[i].distance;
                list[pos].in.push_back(e);
                
                strcpy(e.parentNode, list[i].name.c_str());
                e.isIn = false;
                strcpy(e.otherNode, list[pos].name.c_str());
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
    strcpy(tmp.parentNode, graph[0].name.c_str());
    tmp.isIn = false;
    tmp.weight = rand()%graph.size();
    strcpy(tmp.otherNode, graph[graph.size()-1].name.c_str());
    tmp.distance = graph[graph.size()-1].distance;
    graph[0].out.push_back(tmp);
    //letting know last that has a edge coming from first
    strcpy(tmp.parentNode, graph[graph.size()-1].name.c_str());
    tmp.isIn = true;
    strcpy(tmp.otherNode, graph[0].name.c_str());
    tmp.distance = graph[0].distance;
    graph[graph.size()-1].in.push_back(tmp);
    //-------------------------------

    if(world.rank()==SPLITTER_ID){
        //std::cout<<"Graph solution started\n";
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
        //std::cout<<world.rank()<<" ended\n";
    }else{
        djikstra_worker(world);
        //std::cout<<world.rank()<<" ended\n";
    }
    return 0;
}
