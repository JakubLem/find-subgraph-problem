#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <cstring>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

using namespace std;

const int MAX_V = 1000;          // Maximum number of vertices
const int MAX_EDGES = 10;        // Maximum number of edges per vertex
const int MAX_GRAPHS = 100000;   // Maximum number of graphs
const int THREADS_PER_BLOCK = 128; // Number of threads per block

class Graph {
public:
    int V; // Number of vertices
    int* adj; // Flattened adjacency lists
    int* degree; // Number of edges per vertex
};

__device__ bool isSubgraphDevice(int V, int* adj, int* degree, int sub_V, int* sub_adj, int* sub_degree, int sub_root, int start_vertex) {
    int to_visit_queue[2 * MAX_V];
    int mapping_keys[MAX_V];
    int mapping_values[MAX_V];
    bool visited[MAX_V] = { false };
    int to_visit_start = 0;
    int to_visit_end = 0;
    int mapping_size = 0;

    to_visit_queue[to_visit_end++] = start_vertex;
    to_visit_queue[to_visit_end++] = sub_root;
    mapping_keys[mapping_size] = sub_root;
    mapping_values[mapping_size++] = start_vertex;
    visited[start_vertex] = true;

    while (to_visit_start < to_visit_end) {
        int curr_v = to_visit_queue[to_visit_start++];
        int curr_sub_v = to_visit_queue[to_visit_start++];

        for (int i = 0; i < sub_degree[curr_sub_v]; ++i) {
            int sub_w = sub_adj[curr_sub_v * MAX_EDGES + i];
            bool matched = false;
            for (int j = 0; j < degree[curr_v]; ++j) {
                int w = adj[curr_v * MAX_EDGES + j];
                int k;
                for (k = 0; k < mapping_size; ++k) {
                    if (mapping_keys[k] == sub_w) {
                        if (mapping_values[k] == w) {
                            matched = true;
                        }
                        break;
                    }
                }
                if (k == mapping_size) {
                    mapping_keys[mapping_size] = sub_w;
                    mapping_values[mapping_size++] = w;
                    to_visit_queue[to_visit_end++] = w;
                    to_visit_queue[to_visit_end++] = sub_w;
                    matched = true;
                    visited[w] = true;
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

__global__ void processGraphsKernel(Graph* graphs, int* results, int count, int* sub_adj, int* sub_degree, int sub_V, int sub_root) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < count) {
        Graph g = graphs[index];
        int occurrences = 0;

        for (int i = 0; i < g.V; ++i) {
            if (g.degree[i] >= sub_degree[sub_root] && isSubgraphDevice(g.V, g.adj, g.degree, sub_V, sub_adj, sub_degree, sub_root, i)) {
                occurrences++;
            }
        }

        results[index] = occurrences;
    }
}

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

        Graph* g = new Graph();
        g->V = max_vertex + 1;
        g->degree = new int[g->V]();
        g->adj = new int[g->V * MAX_EDGES]();

        for (const auto& edge : edges) {
            int from = edge.first;
            int to = edge.second;
            g->adj[from * MAX_EDGES + g->degree[from]++] = to;
        }

        // cout << "Loaded graph with " << g->V << " vertices." << endl;
        // for (int i = 0; i < g->V; ++i) {
        //     cout << "Vertex " << i << " degree: " << g->degree[i] << endl;
        // }

        graphs[count++] = g;
    }
}

void prepareAndCopyGraphsToGPU(Graph** hostGraphs, Graph** deviceGraphs, int graphCount) {
    cudaMalloc(deviceGraphs, graphCount * sizeof(Graph));

    for (int i = 0; i < graphCount; i++) {
        Graph g = *hostGraphs[i];
        cudaMalloc(&(g.adj), g.V * MAX_EDGES * sizeof(int));
        cudaMemcpy(g.adj, hostGraphs[i]->adj, g.V * MAX_EDGES * sizeof(int), cudaMemcpyHostToDevice);

        cudaMalloc(&(g.degree), g.V * sizeof(int));
        cudaMemcpy(g.degree, hostGraphs[i]->degree, g.V * sizeof(int), cudaMemcpyHostToDevice);

        cudaMemcpy(&((*deviceGraphs)[i]), &g, sizeof(Graph), cudaMemcpyHostToDevice);
        // cout << "Copied graph " << i + 1 << " to GPU." << endl;
    }
}

int findRoot(int* degree, int V) {
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

int main() {
    const string inputFileName = "data.txt";
    Graph* hostGraphs[MAX_GRAPHS];
    int graphCount = 0;
    loadGraphs(inputFileName, hostGraphs, graphCount);

    Graph* deviceGraphs;
    prepareAndCopyGraphsToGPU(hostGraphs, &deviceGraphs, graphCount);

    // Define the subgraph
    int sub_V = 8;
    int sub_degree[MAX_V] = {0};
    int sub_adj[MAX_V * MAX_EDGES] = {0};

    vector<pair<int, int>> sub_edges = {
        {3, 4}, {4, 5}, {3, 2}, {2, 1}, {3, 6}, {6, 7}
    };

    for (const auto& edge : sub_edges) {
        int from = edge.first;
        int to = edge.second;
        sub_adj[from * MAX_EDGES + sub_degree[from]++] = to;
    }

    int sub_root = findRoot(sub_degree, sub_V);

    int* device_sub_adj;
    int* device_sub_degree;

    cudaMalloc((void**)&device_sub_adj, sub_V * MAX_EDGES * sizeof(int));
    cudaMemcpy(device_sub_adj, sub_adj, sub_V * MAX_EDGES * sizeof(int), cudaMemcpyHostToDevice);

    cudaMalloc((void**)&device_sub_degree, sub_V * sizeof(int));
    cudaMemcpy(device_sub_degree, sub_degree, sub_V * sizeof(int), cudaMemcpyHostToDevice);

    int* results = new int[graphCount];
    int* deviceResults;
    cudaMalloc((void**)&deviceResults, graphCount * sizeof(int));

    dim3 blocks((graphCount + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK);
    dim3 threads(THREADS_PER_BLOCK);

    auto start = chrono::high_resolution_clock::now();
    processGraphsKernel<<<blocks, threads>>>(deviceGraphs, deviceResults, graphCount, device_sub_adj, device_sub_degree, sub_V, sub_root);
    auto finish = chrono::high_resolution_clock::now();

    cudaMemcpy(results, deviceResults, graphCount * sizeof(int), cudaMemcpyDeviceToHost);
    ofstream file("results.txt");
    // cout << "Total graphs: " << graphCount << endl;
    for (int i = 0; i < graphCount; i++) {
        // cout << "Graph " << (i + 1) << " occurrences: " << results[i] << endl;
        file << "Found " << results[i] << " occurrences of subgraph in main graph " << (i + 1) << "." << endl;
    }

    auto durationMs = chrono::duration_cast<chrono::microseconds>(finish - start);
    std::cout << "Program time: " << durationMs.count() << " microseconds." << std::endl;

    cudaFree(deviceGraphs);
    cudaFree(deviceResults);
    cudaFree(device_sub_adj);
    cudaFree(device_sub_degree);
    delete[] results;
    for (int i = 0; i < graphCount; i++) {
        delete[] hostGraphs[i]->adj;
        delete[] hostGraphs[i]->degree;
        delete hostGraphs[i];
    }

    return 0;
}
