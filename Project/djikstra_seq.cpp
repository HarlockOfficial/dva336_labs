#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <chrono>

struct Node;

struct Edge{
    long long int weight;
    Node* n;
};

struct Node{
    std::vector<Edge> in, out; 
    std::string name;   //unique identifier of a Node
    unsigned long long int distance; //default infinity
    Node* nearestPrev;   //default NULL
};

void dijkstra(std::vector<Node> &start){
    std::queue<Node> queue;
    queue.push(start[0]);  //start enters the queue
    while(!queue.empty()){
        Node current = queue.front();    //get first node
        queue.pop(); //removes first node
        for(long unsigned int i = 0;i<current.out.size();++i){    //for all neighbor of the node
            unsigned long long int tmpDistance = current.distance+current.out[i].weight;
            if(tmpDistance<current.out[i].n->distance){
                current.out[i].n->distance=tmpDistance;
                current.out[i].n->nearestPrev=&current;
                queue.push(*current.out[i].n);
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
    srand(123456);
    std::vector<Node> graph;
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

    auto funct_start = std::chrono::steady_clock::now();
    dijkstra(graph);
    auto funct_end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start);
    std::cout<<"Time required by djikstra_seq: "<<duration.count()<<" milliseconds\n";
    for(long unsigned int i = 0 ;i<graph.size();++i){
        std::cout<<"node "<<i<<"\n";
        std::cout<<graph[i].name<<" prev:";
        if(graph[i].nearestPrev==NULL){
            std::cout<<"No nearest prev";
        }else{
            std::cout<<graph[i].nearestPrev->name;
        }
        std::cout<<" distance:"<<graph[i].distance<<"\n";
        std::cout<<"\n--------------------------------------------------\n";
        std::cout<<"out nodes: \n";
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