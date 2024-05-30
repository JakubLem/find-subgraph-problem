import random

def generate_graph(num_vertices=400, num_degree_2=44, num_degree_4=6):
    edges = set()
    vertices = list(range(1, num_vertices + 1))
    random.shuffle(vertices)

    # Przypisz stopnie 4 losowo wybranym wierzchołkom
    degree_4_vertices = random.sample(vertices, num_degree_4)
    
    # Dla wierzchołków czwartego stopnia
    for v in degree_4_vertices:
        while len([x for x, y in edges if x == v or y == v]) < 4:
            target = random.choice(vertices)
            if target != v and (v, target) not in edges and (target, v) not in edges:
                edges.add((v, target))

    # Dla pozostałych wierzchołków drugiego stopnia
    for v in vertices:
        if v not in degree_4_vertices:
            while len([x for x, y in edges if x == v or y == v]) < 2:
                target = random.choice(vertices)
                if target != v and (v, target) not in edges and (target, v) not in edges:
                    edges.add((v, target))

    return edges

# Generuj 100 grafów
num_graphs = 500
all_graphs = []
for _ in range(num_graphs):
    graph_edges = generate_graph()
    all_graphs.append(graph_edges)

# Zapisz do pliku
with open('data.txt', 'w') as file:
    for graph in all_graphs:
        line = ' '.join(f"{x} {y}" for x, y in graph)
        file.write(line + "\n")