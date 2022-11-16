#include <iostream>
#include <string>
#include <vector>
#include <set>
using namespace std;

enum Operation {add, mult, var, constant};

struct Node {
  Operation op;
  Node* op_a;
  Node* op_b;
  
  char var_val;
  int const_val;
};

class BruteForce {
public:
  vector<int> n_set;
  int depth;
  int prev_start;
  vector<Node*> treesSoFar;
  
  BruteForce(vector<int> n_set) {
    depth = 0;
    treesSoFar = {};
    treesSoFar.push_back(new Node({.op = var,
                                   .op_a = nullptr,
                                   .op_b = nullptr,
                                   .var_val = 'x',
                                   .const_val = 0}));

    for (int i = 0; i < n_set.size(); i++) {
      treesSoFar.push_back(new Node({.op = constant,
                            .op_a = nullptr,
                            .op_b = nullptr,
                            .var_val = '\0',
                            .const_val = n_set[i]}));
                            
    }
    prev_start = 0;
  }

  int grow_to(int max_depth, bool perm_lock) {
    if (depth >= max_depth) {
      return 0;
    }
    int growth = 0;
    while (depth < max_depth) {
      vector<Node*> newTrees = {};

      for (Operation op: {add, mult}) {
        for (int t = prev_start; t < treesSoFar.size(); t++) {
          for (int tau = 0; tau < t; tau++) {
              
            newTrees.push_back(new Node{.op = op,
                         .op_a = treesSoFar[t],
                         .op_b = treesSoFar[tau],
                         .var_val = '\0',
                         .const_val = 0});
          }
        }
      }

      prev_start = treesSoFar.size();

      treesSoFar.insert(treesSoFar.end(), newTrees.begin(), newTrees.end());
      growth += newTrees.size();

      depth += 1;
    }
    return growth;
  }
}; 

void printTree(Node* root, int depth) {
  for (int i = 0; i < depth; i++) {
    cout << ' ';
  }

  if (root->op == add) {
    cout << "+" << endl;
  } else if (root->op == mult) {
    cout << "*" << endl;
  } else if (root->op == var) {
    cout << "x" << endl;
  } else {
    cout << root->const_val << endl;
  }

  if (root->op_a) {
    printTree(root->op_a, depth + 1);
  }
  if (root->op_b) {
    printTree(root->op_b, depth + 1);
  }
}

bool treeEqual(Node* a, Node* b) {
  if (!a && !b) {
    return true;
  }
  if (!a || !b) {
    return false;
  }
  return a->op == b->op && a->var_val == b->var_val && a->const_val == b->const_val && treeEqual(a->op_a, b->op_a) && treeEqual(a->op_b, b->op_b);
}

int main() {
  BruteForce bf = BruteForce({1, 2});

  for (int i = 0; i < 4; i++) {
    bf.grow_to(i, false);
    cout << i << endl;
    cout << bf.treesSoFar.size() << endl;
  }

  for (int i = 0; i < bf.treesSoFar.size(); i++) {
    cout << "NEW TREE" << endl;
    printTree(bf.treesSoFar[i], 0);
  }
  
  return 0;
}


