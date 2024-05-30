#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <map>
#include <queue>
#include <cstring>

using namespace std;
using namespace std::chrono;

const int MAX_GRAPHS = 100000;
const int MAX_VERTICES = 1000;
const int MAX_EDGES = 10;

class Graph {
private:
    int V;
    int** adj;
    int* degree;

public:
    Graph(int V) : V(V) {
        adj = new int*[V];
        degree = new int[V];
        for (int i = 0; i < V; ++i) {
            adj[i] = new int[MAX_EDGES];
            degree[i] = 0;
        }
    }

    ~Graph() {
        for (int i = 0; i < V; ++i) {
            delete[] adj[i];
        }
        delete[] adj;
        delete[] degree;
    }

    void addEdge(int v, int w) {
        if (v >= V || w >= V) {
            return;
        }
        if (degree[v] >= MAX_EDGES) {
            return;
        }
        adj[v][degree[v]++] = w;
    }

    void printGraph() const {
        for (int i = 0; i < V; ++i) {
            if (degree[i] > 0) {
                for (int j = 0; j < degree[i]; ++j) {
                    cout << adj[i][j] << " ";
                }
                cout << endl;
            }
        }
    }

    int findRoot() const {
        int max_degree = 0;
        int root = -1;
        for (int i = 0; i < V; ++i) {
            if (degree[i] > max_degree) {
                max_degree = degree[i];
                root = i;
            }
        }
        return root;
    }

    bool isSubgraph(int v, int sub_v, Graph& sub) {
        queue<pair<int, int>> to_visit;
        map<int, int> mapping;
        to_visit.push({v, sub_v});
        mapping[sub_v] = v;

        while (!to_visit.empty()) {
            int curr_v = to_visit.front().first;
            int curr_sub_v = to_visit.front().second;
            to_visit.pop();

            for (int i = 0; i < sub.degree[curr_sub_v]; ++i) {
                int sub_w = sub.adj[curr_sub_v][i];
                bool matched = false;
                for (int j = 0; j < degree[curr_v]; ++j) {
                    int w = adj[curr_v][j];
                    if (mapping.find(sub_w) == mapping.end()) {
                        mapping[sub_w] = w;
                        to_visit.push({w, sub_w});
                        matched = true;
                        break;
                    } else if (mapping[sub_w] == w) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    return false;
                }
            }
        }
        return true;
    }

    int countSubgraphs(Graph& sub) {
        int count = 0;
        int sub_root = sub.findRoot();
        int sub_root_degree = sub.degree[sub_root];

        for (int i = 0; i < V; ++i) {
            if (degree[i] < sub_root_degree) {
                continue;
            }

            if (isSubgraph(i, sub_root, sub)) {
                count++;
            }
        }
        return count;
    }

    int getVertexCount() const {
        return V;
    }
};

void loadGraphs(const string& filename, Graph* graphs[], int& count) {
    ifstream file(filename);
    string line;
    count = 0;

    while (getline(file, line) && count < MAX_GRAPHS) {
        stringstream ss(line);
        int u, v;
        vector<pair<int, int>> edges;
        int max_vertex = -1;

        while (ss >> u >> v) {
            edges.push_back({u, v});
            max_vertex = max(max_vertex, max(u, v));
        }

        Graph* g = new Graph(max_vertex + 1);
        for (const auto& edge : edges) {
            g->addEdge(edge.first, edge.second);
        }

        graphs[count++] = g;
    }
}

void saveResults(const string& filename, const string results[], int count) {
    ofstream file(filename);
    for (int i = 0; i < count; i++) {
        file << results[i] << endl;
    }
}

int main() {
    Graph* graphs[MAX_GRAPHS];
    int graphCount = 0;

    loadGraphs("data.txt", graphs, graphCount);

    Graph sub(MAX_VERTICES);
    sub.addEdge(3, 4);
    sub.addEdge(4, 5);
    sub.addEdge(3, 2);
    sub.addEdge(2, 1);
    sub.addEdge(3, 6);
    sub.addEdge(6, 7);

    string* results = new string[graphCount];

    auto start = high_resolution_clock::now();  // Rozpoczęcie pomiaru czasu
    for (int i = 0; i < graphCount; i++) {
        int occurrences = graphs[i]->countSubgraphs(sub);
        results[i] = "Found " + to_string(occurrences) + " occurrences of subgraph in main graph " + to_string(i + 1) + ".";
        delete graphs[i]; // Clean up memory
    }

    auto stop = high_resolution_clock::now();  // Zakończenie pomiaru czasu
    auto duration = duration_cast<microseconds>(stop - start);

    // Save results to file
    saveResults("result.txt", results, graphCount);

    delete[] results; // Clean up results array

    cout << "Total time taken for all iterations: " << duration.count() << " microseconds" << endl;
    cout << "Average time per iteration: " << duration.count() / graphCount << " microseconds" << endl;

    return 0;
}
