/*
Optimizes stochastic.cpp by removing the poly_set operations
Storing operands as pointers instead of in a vector
Moving shared variables to the top of the program
Storing costs as constants instead of in a map
Passing large object as parameters with const ref

Switching set to unordered_set didn't often have a noticeable effect, 
but if it did it usually made the runtime slower
*/

#include "polynomial.h"
#include <set>
#include <limits>
#include <random>
#include <cmath>
#include <algorithm>
using namespace std;

random_device rd; 
mt19937 gen(rd()); 

int id_counter = 0;

enum Operation {add, mult, var, constant};

const float MULT_COST = 1;
const float ADD_COST = 0.25;

struct Node {
  Operation op;
  int arg;
  int val;
  Node* op0;
  Node* op1;
  int id;
  Polynomial poly;
  set<int> add_set;
  set<int> mult_set;
};

struct Circuit {
  Node root;
  float cost;
  vector<Node> nodes;
};

struct PrioritizedCircuit {
  float priority;
  Circuit circuit;
  float dist;
};

struct pc_less {
  bool operator () (PrioritizedCircuit const& a, PrioritizedCircuit const& b) const {
    if (a.priority < b.priority) return true;
    if (a.priority > b.priority) return false;

    return false;
  }
};

Node get_leaf(int n_var, int arg, int val) {
  Polynomial poly = Polynomial(n_var);
  vector<int> powers(n_var, 0); // initialize vector to all 0

  Operation op;
  if (arg != -1) {
    powers[arg] = 1;
    poly.poly_map[powers] = 1;
    op = var;
  } else {
    poly.poly_map[powers] = val;
    op = constant;
  }

  id_counter += 1;
  Node leaf = {op, arg, val, nullptr, nullptr, id_counter, poly, {}, {}};
  return leaf;
}

class Stochastic {
  public:
  Polynomial target;
  int n_var;
  vector<Node> leaves;
  int max_val;

  Stochastic(const Polynomial &poly, int n_vals) {
    target = poly;
    n_var = poly.n_var;

    for (int i = 0; i < n_var; i++) {
      leaves.push_back(get_leaf(n_var, i, 0));
    }
    for (int i = 1; i < n_vals + 1; i++) {
      leaves.push_back(get_leaf(n_var, -1, i));
    }
  }

  Circuit blank_circuit(const vector<Circuit> &models) {
    set<int> seen_ids = {};
    vector<Node> nodes = {};
    for (int i = 0; i < leaves.size(); i++) {
      nodes.push_back(leaves[i]);
      seen_ids.insert(leaves[i].id);
    }

    for (int i = 0; i < models.size(); i++) {
      for (int j = 0; j < models[i].nodes.size(); j++) {
        int curr_id = models[i].nodes[j].id;
        if ((!seen_ids.count(curr_id)) &&  (models[i].root.add_set.count(curr_id) || models[i].root.mult_set.count(curr_id))) {
          seen_ids.insert(curr_id);
          nodes.push_back(models[i].nodes[j]);
        }
      }
    }

    uniform_int_distribution<> distr(0, nodes.size() - 1);
    Circuit newCirc = {nodes[distr(gen)], 0, nodes};
    return newCirc;
  }

  float get_pred(const Circuit &circuit, bool simple) {
    Polynomial r = circuit.root.poly;

    int curr_max = 0;
    set<vector<int>> s_a = {};
    for (auto const& [key, val] : r.poly_map) {
      if (val > curr_max) {
        curr_max = val;
      }
      s_a.insert(key);
    }
    if ((r.poly_map.size() > target.poly_map.size()) || (curr_max > max_val)) {
      return 1000000;
    }

    Polynomial q = target - r;
    float d_plus = 0;
    for (auto const& [key, val] : q.poly_map) {
      if (val == 0) continue;
      if (simple && val < 0) {
        return 1000000;
      }
      else {
        d_plus += sqrt(abs(val));
      }
    }

    set<vector<int>> s_p = {};
    for (auto const& [key, val] : target.poly_map) {
      s_p.insert(key);
    }
    
    set<vector<int>> unique1;
    set_difference(s_a.begin(), s_a.end(), s_p.begin(), s_p.end(),
      inserter(unique1, unique1.end()));
    set<vector<int>> unique2;
    set_difference(s_p.begin(), s_p.end(), s_a.begin(), s_a.end(),
      inserter(unique2, unique2.end()));

    // getting all the terms that are unique to target and r
    set<vector<int>> unique;
    set_union(unique1.begin(), unique1.end(), unique2.begin(), unique2.end(),
      inserter(unique, unique.end()));

    vector<int> temp_sums = {};
    for (const auto& vec : unique) {
      int sum = 0;
      for (const auto& elem : vec) {
        sum += elem;
      }
      temp_sums.push_back(sum);
    }
    float d_x = 0;
    for (const auto& elem : temp_sums) {
      d_x += elem;
    }

    float cost = d_plus + d_x;
    return cost;
  }

  Circuit create_new(Circuit circuit, const Operation &op, Node* op0, Node* op1, bool track_sets) {
    id_counter += 1;
    int id = id_counter;

    set<int> add_set;
    set<int> mult_set;
    
    if (track_sets) {
      set_union(op0->add_set.begin(), op0->add_set.end(), op1->add_set.begin(), op1->add_set.end(),
        inserter(add_set, add_set.end()));
      set_union(op0->mult_set.begin(), op0->mult_set.end(), op1->mult_set.begin(), op1->mult_set.end(),
        inserter(mult_set, mult_set.end()));

      if (op == add) {
        add_set.insert(id);
      } else {
        mult_set.insert(id);
      }
    }

    Polynomial newPoly;
    if (op == add) {
      newPoly = op0->poly + op1->poly;
    } else {
      newPoly = op0->poly * op1->poly;
    }

    Node newNode = {op, -1, 0, op0, op1, id_counter, newPoly, add_set, mult_set};

    float cost = 0;
    if (op == mult) {
      cost = circuit.cost + MULT_COST;
    } else {
      cost = circuit.cost + ADD_COST;
    }
    if (track_sets) {
      cost = ADD_COST * newNode.add_set.size() + MULT_COST * newNode.mult_set.size();
    }
    
    circuit.nodes.push_back(newNode);
    Circuit newC = {newNode, cost, circuit.nodes};

    return newC;
  }

  Circuit sample_search(int max_iters, int max_cost, float alpha, float gamma, bool verbose, bool use_pred, int n_models, bool wrapped) {
    Circuit best;
    bool soln = false;
    
    int solutions_found = 0;

    Circuit curr = blank_circuit({});
    
    float prev_pred = get_pred(curr, false);
    vector<PrioritizedCircuit> models;

    int total_iters = 0;

    for (int i = 0; i < max_iters; i++) {
      if (verbose) {
        if ((i % 1000) == 0) {
          cout << "total iters " << total_iters << endl;
          cout << "iteration: " << i + 1 << "/" << max_iters << endl;
          cout << "best model: ";
          if (models.size() == 0) {
            cout << "None" << endl;
          } else {
            models[0].circuit.root.poly.print();
          } 
          if (soln) {
            cout << "solution cost: " << best.cost << endl;
          } else {
            cout << "solution cost: None" << endl;
          }
          
          cout << "solutions found: " << solutions_found << endl;
          cout << "current cost: " << curr.cost << endl;
          cout << "current prediciton: " << prev_pred << endl;
          curr.root.poly.print();
          cout << endl;
        }
      }

      vector<Circuit> circs = {};
      vector<float> preds = {};
      vector<float> weights = {};
      vector<PrioritizedCircuit> potential_models = {};

      for (int n = 0; n < curr.nodes.size(); n++) {
        for (auto const& oper : {add, mult}) {
          total_iters += 1;
          Circuit newCirc = create_new(curr, oper, &curr.root, &curr.nodes[n], n_models > 0);

          int new_max = 0;
          for (auto const& [key, val] : newCirc.root.poly.poly_map) {
            if (abs(val) > 0) {
              new_max = abs(val);
              break;
            }
          }
          if (new_max == 0) continue;

          if (newCirc.root.poly.poly_map == target.poly_map) {
            if (!soln || newCirc.cost < best.cost) {
              soln = true;
              best = newCirc;
              if (n_models > 0) {
                PrioritizedCircuit pc = {newCirc.cost, newCirc, 0};
                potential_models.push_back(pc);
              }
            }
            solutions_found += 1;
          } else if (!(newCirc.cost >= max_cost || (soln && newCirc.cost >= best.cost - 1))) {
            float pred = get_pred(newCirc, false);
            if (pred < 1000000) {
              circs.push_back(newCirc);
              preds.push_back(pred);
              weights.push_back(1.0/pow(newCirc.cost - curr.cost + alpha * pred, gamma));

              float priority = newCirc.cost + alpha * pred; 
              if (wrapped) {
                priority = newCirc.cost + 1000 * get_pred(newCirc, true);
              }
              if (n_models > 0 && (models.size() < n_models || priority < models.back().priority)) {
                PrioritizedCircuit pc = {priority, newCirc, pred};
                potential_models.push_back(pc);
              }
            }
          }
        }
      }
      if (potential_models.size() > 0 && n_models > 0) {
        models.insert(models.end(), potential_models.begin(), potential_models.end());
        vector<PrioritizedCircuit> new_models = models;
        sort(new_models.begin(), new_models.end(), pc_less());
        models = {};

        set<float> seen_pri;
        int min_val;
        if (n_models < new_models.size()) {
          min_val = n_models;
        } else {
          min_val = new_models.size();
        }
        for (int k = 0; k < min_val; k++) {
          if (!seen_pri.count(new_models[k].priority)) {
            seen_pri.insert(new_models[k].priority);
            models.push_back(new_models[k]);
          }
        }
      }
      if (circs.size() == 0) {
        vector<Circuit> modelCircuits = {};
        for (int k = 0; k < models.size(); k++) {
          modelCircuits.push_back(models[k].circuit);
        }
        curr = blank_circuit(modelCircuits);
      } else {
        discrete_distribution<> d(weights.begin(), weights.end());
        
        int choice = d(gen);
        curr = circs[choice];
        prev_pred = preds[choice];
      }

    }
    if (wrapped && !soln) {
      // use -100 to flag this is not a solution
      return {{}, -100, {}};
    }
    return best;
  }
};

int main() {
  Polynomial poly = Polynomial(3);
  poly.new_term({1, 0, 0}, 1);
  poly.new_term({0, 1, 0}, 1);
  poly.new_term({0, 0, 1}, 1);
  poly.print();

  Stochastic engine = Stochastic(poly, 8);
  Circuit sol = engine.sample_search(10000, 10, 1, 1, false, true, 3, true);

  cout << "Target: ";
  poly.print();
  if (sol.cost == -100) {
    cout << "NO SOLUTION" << endl;
  } else {
    cout << "Solution: ";
    sol.root.poly.print();
    cout << "Additions: " << sol.root.add_set.size() << endl;
    cout << "Multiplications: " << sol.root.mult_set.size() << endl;
  }

  return 0;
}