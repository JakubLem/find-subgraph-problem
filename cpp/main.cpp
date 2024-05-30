#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <map>
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
            // cerr << "Vertex index out of bounds." << endl;
            return;
        }
        if (degree[v] >= MAX_EDGES) {
            // cerr << "Maximum degree reached for vertex " << v << "." << endl;
            return;
        }
        adj[v][degree[v]++] = w;
    }

    void printGraph() const {
        for (int i = 0; i < V; ++i) {
            if (degree[i] > 0) {
                // cout << "Vertex " << i << " has edges to: ";
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

    bool isSubgraph(int v, int sub_v, map<int, int>& mapping, Graph& sub) {
        if (mapping.find(sub_v) != mapping.end()) {
            return mapping[sub_v] == v;
        }
        mapping[sub_v] = v;

        for (int i = 0; i < sub.degree[sub_v]; ++i) {
            int sub_w = sub.adj[sub_v][i];
            bool matched = false;
            for (int j = 0; j < degree[v]; ++j) {
                int w = adj[v][j];
                if (isSubgraph(w, sub_w, mapping, sub)) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                return false;
            }
        }
        return true;
    }

    int countSubgraphs(Graph& sub) {
        int count = 0;
        int sub_root = sub.findRoot();
        int sub_root_degree = sub.degree[sub_root];

        // cout << "Checking vertices in the main graph with degree >= " << sub_root_degree << " (degree of subgraph root)" << endl;

        for (int i = 0; i < V; ++i) {
            if (degree[i] < sub_root_degree) {
                // cout << "Vertex " << i << " rejected | Degree: " << degree[i] << endl;
                continue;
            }

            // cout << "Vertex " << i << " checking | ";

            map<int, int> mapping;
            if (isSubgraph(i, sub_root, mapping, sub)) {
                // cout << "true" << endl;
                count++;
            } else {
                // cout << "false" << endl;
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

    // cout << "Subgraph: " << endl;
    // sub.printGraph();
    // cout << endl;

    string* results = new string[graphCount];

    auto start = high_resolution_clock::now();  // Rozpoczęcie pomiaru czasu
    for (int i = 0; i < graphCount; i++) {
        // cout << "Main Graph " << i + 1 << ": " << endl;
        // graphs[i]->printGraph();
        // cout << endl;
        int occurrences = graphs[i]->countSubgraphs(sub);
        // cout << "Found " << occurrences << " occurrences of subgraph in main graph " << i + 1 << "." << endl << endl;
        // Save result to the results array
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
