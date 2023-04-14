#!/usr/bin/env python3
import re
import sys

import pydot

DOT_PATH = ""
PROJECT_NAME = DOT_PATH.split("/")[-1].split(".")[0]

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: python3 utils/pruneCG.py [dot file]")
        sys.exit()
    else:
        DOT_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: python3 utils/pruneCG.py [dot file]")
    sys.exit()


dot = pydot.graph_from_dot_file(DOT_PATH)

dot = pydot.graph_from_dot_file(DOT_PATH)[0]

print(len(dot.get_nodes()), len(dot.get_edges()))

# Find all unused nodes' names
unused_nodes_names = set()
for node in dot.get_nodes():
    if PROJECT_NAME not in node.get_label() or PROJECT_NAME + ".." in node.get_label():
        dot.del_node(node)
        unused_nodes_names.add(node.get_name())

# Delete edges connected to unused nodes
for edge in dot.get_edges():
    if edge.get_source() in unused_nodes_names or edge.get_destination() in unused_nodes_names:
        dot.del_edge((edge.get_source(), edge.get_destination()))

print(len(dot.get_nodes()), len(dot.get_edges()))

# Delete dangling nodes & rename all nodes
used_nodes_names = set()
for edge in dot.get_edges():
    used_nodes_names.add(edge.get_source())
    used_nodes_names.add(edge.get_destination())
for node in dot.get_nodes():
    if node.get_name() not in used_nodes_names:
        dot.del_node(node)
    node.set_label('"{' + re.sub("\\d+", "::", node.get_label()[6:-22]) + '}"')

print(len(dot.get_nodes()), len(dot.get_edges()))

dot.set_name('"Call Graph - ' + PROJECT_NAME + '"')
dot.set_label("Call Graph - " + PROJECT_NAME)

dot.write("Call Graph - " + PROJECT_NAME + ".svg", format="svg")
