#include <iostream>
#include <string>
#include <vector>
using namespace std;

enum Operation {add, mult, var, constant};

struct Node {
  Operation op; // can be an operation (either + or *), a variable, or a constant
  Node* op_a; // child node
  Node* op_b; // child node
  
  char var_val;
  int const_val;
};

class BruteForce {
public:
  vector<int> n_set; // constants allowed in the tree
  int depth;
  int prev_start;
  vector<Node*> treesSoFar;
  
  BruteForce(vector<int> n_set) {
    depth = 0;
    treesSoFar = {};
    prev_start = 0;

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
  }

  int grow_to(int max_depth) {
    if (depth >= max_depth) {
      return 0;
    }
    int growth = 0;
    while (depth < max_depth) {
      vector<Node*> newTrees = {};

      // generate trees where the two children of the root are the same
      vector<Node*> sameChild = {};
      for (Operation op: {add, mult}) {
        for (int t = prev_start; t < treesSoFar.size(); t++) {
          sameChild.push_back(new Node{.op = op,
                         .op_a = treesSoFar[t],
                         .op_b = treesSoFar[t],
                         .var_val = '\0',
                         .const_val = 0});
        }
      }

      // generate trees where the two children of the root are different
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
      treesSoFar.insert(treesSoFar.end(), sameChild.begin(), sameChild.end());
      growth += newTrees.size();

      depth += 1;
    }
    return growth;
  }
}; 

void printTreeHelper(Node* root, int depth) {
  for (int i = 0; i < depth; i++) {
    cout << ' ';
  }

  if (root->op == add) {
    cout << "+" << endl;
  } else if (root->op == mult) {
    cout << "*" << endl;
  } else if (root->op == var) {
    cout << root->var_val << endl;
  } else {
    cout << root->const_val << endl;
  }

  if (root->op_a) {
    printTreeHelper(root->op_a, depth + 1);
  }
  if (root->op_b) {
    printTreeHelper(root->op_b, depth + 1);
  }
}

void printTree(Node* root) {
  printTreeHelper(root, 0);
}

int factorialPoly(int x) {
  return (x + 1) * (x + 2) * (x + 3);
}

int run(Node* root, int input) {
  if (root->op == add) {
    return run(root->op_a, input) + run(root->op_b, input); 
  } else if (root->op == mult) {
    return run(root->op_a, input) * run(root->op_b, input); 
  } else if (root->op == var) {
    return input;
  } else {
    return root->const_val;
  }
}

// determine the number of multiplication operations in the tree
// used to evaluate each tree
int countMult(Node* root) {
  if (root == nullptr) {
    return 0;
  } else if (root->op == mult) {
    return countMult(root->op_a) + countMult(root->op_b) + 1;
  } else {
    return countMult(root->op_a) + countMult(root->op_b);
  }
}

int main() {
  BruteForce bf = BruteForce({1, 2});

  cout << "Total trees at each level: " << endl;
  for (int i = 0; i < 4; i++) {
    bf.grow_to(i);
    cout << i << " " << bf.treesSoFar.size() << endl;
  }
  cout << endl;

  vector<Node*> bestTrees = {};
  int bestMultCount = 0;
  bool foundValid = false;

  for (int i = 0; i < bf.treesSoFar.size(); i++) {
    bool validTree = true;

    for (int j = 0; j < 6; j++) {
      if (factorialPoly(j) != run(bf.treesSoFar[i], j)) {
        validTree = false;
        break;
      }
    }
    
    if (validTree) {
      if (!foundValid) {
        foundValid = true;
        bestMultCount = countMult(bf.treesSoFar[i]);
        bestTrees.push_back(bf.treesSoFar[i]);
      } else if (countMult(bf.treesSoFar[i]) == bestMultCount) {
        bestTrees.push_back(bf.treesSoFar[i]);
      } else if (countMult(bf.treesSoFar[i]) < bestMultCount) {
        bestMultCount = countMult(bf.treesSoFar[i]);
        bestTrees = {};
        bestTrees.push_back(bf.treesSoFar[i]);
      }
    }
  }
  
  cout << "Best trees: " << endl;
  for (int i = 0; i < bestTrees.size(); i++) {
    printTree(bestTrees[i]);
    cout << endl;
  }
  cout << "Number of best trees: " << bestTrees.size() << endl;
  return 0;
}
