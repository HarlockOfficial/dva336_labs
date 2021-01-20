#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <climits>
#include <chrono>
#include <string>
#include <thread>
#include <cstring>

class Edge{
    public:
        //unique identifier of node that owns this edge
        char parentNode[21];
        //weight of the edge
        unsigned long long int weight;
        //unique identifier of other node
        char otherNode[21];
};

class Node{
    public:
        std::vector<Edge> out = std::vector<Edge>();
        std::string name;   //unique identifier of a Node
        unsigned long long int distance{}; // from start
        bool operator< (const Node& node2) const{
            return this->distance>node2.distance;
        }
        bool operator> (const Node& node2) const{
            return this->distance<node2.distance;
        }
};


void dijkstra(std::vector<Node> &start){
    std::priority_queue<Node, std::vector<Node>, std::less<> > queue;
    queue.push(start[0]);  //start enters the queue
    while(!queue.empty()){
        Node current = queue.top();    //get first node
        queue.pop(); //removes first node
        for(long unsigned int i = 0;i<current.out.size();++i){    //for all neighbor of the node
            Node* destination_node;
            for (Node &tmp: start) {
                if (strncmp(current.out[i].otherNode, tmp.name.c_str(), 21) == 0) {
                    destination_node = &tmp;
                    break;
                }
            }
            unsigned long long int tmpDistance = current.distance+current.out[i].weight;
            if(tmpDistance<destination_node->distance){
                destination_node->distance = tmpDistance;
                queue.push(*destination_node);
            }
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
                bool seen = false;
                for(const Edge& tmp_edge: list[i].out){
                    if(strcmp(e.parentNode, tmp_edge.parentNode)==0 && strcmp(e.otherNode, tmp_edge.otherNode)==0){
                        seen = true;
                        break;
                    }
                }
                if(!seen) {
                    list[i].out.push_back(e);
                }
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
    if(graph[0].out.size()==0) {
        // start must have at least one edge
        Edge tmp{};
        strcpy(tmp.parentNode, graph[0].name.c_str());
        tmp.weight = rand() % graph.size();
        strcpy(tmp.otherNode, graph[graph.size() - 1].name.c_str());
        graph[0].out.push_back(tmp);
    }
    auto funct_start = std::chrono::steady_clock::now();
    dijkstra(graph);
    auto funct_end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start);
    std::cout<<"Time required by djikstra_seq: "<<duration.count()<<" milliseconds\n";
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
    return 0;
}