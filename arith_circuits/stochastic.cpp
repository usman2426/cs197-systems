/*

set union difference, getting a random number

how i compare for PC

auto const????
*/

#include "polynomial.h"
#include <set>
#include <map>
#include <limits>
#include <random>
#include <cmath>
#include <algorithm>
using namespace std;

enum Operation {add, mult, var, constant};

struct Node {
  Operation op;
  int arg;
  int val;
  vector<Node> operands;
  int id;
  Polynomial poly;
  set<int> add_set;
  set<int> mult_set;
  set<vector<int>> poly_set;
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
  
  random_device rd; 
  mt19937 gen(rd()); 
  uniform_int_distribution<> distr(0, INT_MAX);
  Node leaf = {op, arg, val, {}, distr(gen), poly, {}, {}, {}};
  return leaf;
}

class Stochastic {
  public:
  Polynomial target;
  int n_var;
  map<Operation, float> op_costs;
  vector<Node> leaves;
  vector<int> constraints;
  int max_val;

  void set_constraints() {
    for (int i = 0; i < n_var; i++) {
      constraints.push_back(0);
    }

    max_val = 0;
    for (auto const& [key, val] : target.poly_map) {
      for (int i = 0; i < n_var; i++) {
        if (key[i] > constraints[i]) {
          constraints[i] = key[i];
        }
      }
      if (val > max_val) {
        max_val = val;
      }
    }
    /*for (int i = 0; i < constraints.size(); i++) {
      cout << constraints[i] << endl;
    }*/
    //cout << max_val << endl;
    //cout << constraints << endl;
  }

  Stochastic(Polynomial poly, int n_vals, map<Operation, float> costs) {
    target = poly;
    n_var = poly.n_var;
    
    op_costs = costs;

    //cout << "constructor " << endl;
    //cout << op_costs[add] << endl;
    //cout << op_costs[mult] << endl;
    //return;

    for (int i = 0; i < n_var; i++) {
      //cout << "here1" << endl;
      leaves.push_back(get_leaf(n_var, i, 0));
    }
    for (int i = 1; i < n_vals + 1; i++) {
      //cout << "here2" << endl;
      leaves.push_back(get_leaf(n_var, -1, i));
    }
    /*for (int i = 0; i < leaves.size(); i++) {
      cout << leaves[i].op << endl;
      cout << leaves[i].arg << endl;
    }*/
    //cout << leaves << endl;
    set_constraints();

    // DELETE BELOWWWWW
    /*Circuit c = blank_circuit({});
    get_pred(c, false);
    cout << "here1" << endl;
    create_new(c, add, {c.root, c.nodes[0]}, true);*/
  }

  Circuit blank_circuit(vector<Circuit> models) {
    
    set<int> seen_ids = {};
    vector<Node> nodes = {};
    //cout << "leaves size " << leaves.size() << endl;
    for (int i = 0; i < leaves.size(); i++) {
      nodes.push_back(leaves[i]);
      seen_ids.insert(leaves[i].id);
    }
    //cout << "no seg fault" << endl;
    for (int i = 0; i < models.size(); i++) {
      for (int j = 0; j < models[i].nodes.size(); j++) {
        int curr_id = models[i].nodes[j].id;
        if ((!seen_ids.count(curr_id)) &&  (models[i].root.add_set.count(curr_id) || models[i].root.mult_set.count(curr_id))) {
          seen_ids.insert(curr_id);
          nodes.push_back(models[i].nodes[j]);
        }
      }
    }

    random_device rd; 
    mt19937 gen(rd()); 
    uniform_int_distribution<> distr(0, nodes.size() - 1);
    Circuit newCirc = {nodes[distr(gen)], 0, nodes};
    return newCirc;
  }

  float get_pred(Circuit circuit, bool simple) {

    //cout << "here2" << endl;
    Polynomial r = circuit.root.poly;
    //r.print();
    //target.print();

    int curr_max = 0;
    set<vector<int>> s_a = {};
    for (auto const& [key, val] : r.poly_map) {
      if (val > curr_max) {
        curr_max = val;
      }
      s_a.insert(key);
    }
    if ((r.poly_map.size() > target.poly_map.size()) || (curr_max > max_val)) {
      /*cout << curr_max << endl;
      cout << max_val << endl;
      cout << r.poly_map.size() << endl;
      cout << target.poly_map.size() << endl;
      cout << "printing here1" << endl;*/
      return 1000000;
    }
    //cout << "here4" << endl;
    Polynomial q = target - r;
    float d_plus = 0;
    for (auto const& [key, val] : q.poly_map) {
      if (val == 0) continue;
      if (simple && val < 0) {
        //cout << val << endl;
        //cout << "printing here" << endl;
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
    
    //cout << "here3" << endl;
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
    //cout << unique.size() << endl;
    set<vector<int>> diff;
    set_difference(unique.begin(), unique.end(), circuit.root.poly_set.begin(), circuit.root.poly_set.end(),
      inserter(diff, diff.end()));

    vector<int> temp_sums = {};
    for (const auto& vec : diff) {
      int sum = 0;
      for (const auto& elem : vec) {
        sum += elem;
      }
      temp_sums.push_back(sum);
      //cout << sum << endl;
    }
    float d_x = 0;
    for (const auto& elem : temp_sums) {
      d_x += elem;
    }

    float cost = d_plus + d_x;
    return cost;
  }

  Circuit create_new(Circuit circuit, Operation op, vector<Node> operands, bool track_sets) {
    random_device rd; 
    mt19937 gen(rd()); 
    uniform_int_distribution<> distr(0, INT_MAX);
    int id = distr(gen);

    set<int> add_set;
    set<int> mult_set;
    //circuit.root.poly.print();
    if (track_sets) {
      set_union(operands[0].add_set.begin(), operands[0].add_set.end(), operands[1].add_set.begin(), operands[1].add_set.end(),
        inserter(add_set, add_set.end()));
      set_union(operands[0].mult_set.begin(), operands[0].mult_set.end(), operands[1].mult_set.begin(), operands[1].mult_set.end(),
        inserter(mult_set, mult_set.end()));

      if (op == add) {
        add_set.insert(id);
      } else {
        mult_set.insert(id);
      }
    }

    //cout << "add size " << add_set.size() << endl;
    //cout << "mult size " << mult_set.size() << endl;

    Polynomial newPoly;
    if (op == add) {
      newPoly = operands[0].poly + operands[1].poly;
    } else {
      newPoly = operands[0].poly * operands[1].poly;
    }

    // building up a union across 4 sets
    set<vector<int>> op0_terms;
    set<vector<int>> op1_terms;
    set<vector<int>> op_union;

    for (auto const& elem : operands[0].poly.poly_map) {
      op0_terms.insert(elem.first);
    }
    for (auto const& elem : operands[1].poly.poly_map) {
      op1_terms.insert(elem.first);
    }

    set_union(op0_terms.begin(), op0_terms.end(), op1_terms.begin(), op1_terms.end(), inserter(op_union, op_union.end()));
    set<vector<int>> op_union1;
    set_union(operands[0].poly_set.begin(), operands[0].poly_set.end(), op_union.begin(), op_union.end(), inserter(op_union1, op_union1.end()));
    set<vector<int>> op_union2;
    set_union(operands[1].poly_set.begin(), operands[1].poly_set.end(), op_union1.begin(), op_union1.end(), inserter(op_union2, op_union2.end()));

    Node newNode = {op, -1, 0, operands, id, newPoly, add_set, mult_set, op_union2};

    float cost = circuit.cost + op_costs[op];
    if (track_sets) {
      /*cout << " checking cost " << endl;;

      cout << "add size after " << newNode.add_set.size() << endl;
      cout << "mult size after " << newNode.mult_set.size() << endl;
      cout << op_costs[add] << endl;
      cout << op_costs[mult] << endl;*/
      cost = op_costs[add] * newNode.add_set.size() + op_costs[mult] * newNode.mult_set.size();
      //cout << cost << endl;
    }
    //cout << "circuit sizing" << endl;
    //cout << circuit.nodes.size() << endl;
    circuit.nodes.push_back(newNode);
    //cout << circuit.nodes.size() << endl;
    Circuit newC = {newNode, cost, circuit.nodes};

    return newC;
  }

  Circuit sample_search(int max_iters, int max_cost, float alpha, float gamma, bool verbose, bool use_pred, int n_models, bool wrapped) {
    Circuit best;
    bool soln = false;
    //cout << "best cost" << best.cost << endl;
    int solutions_found = 0;

    Circuit curr = blank_circuit({});
    
    float prev_pred = get_pred(curr, false);
    //cout << prev_pred << endl;
    //return best;
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
      
      //cout << "nodes size " << curr.nodes.size() << endl;

      for (int n = 0; n < curr.nodes.size(); n++) {
        for (auto const& oper : {add, mult}) {
          total_iters += 1;
          //curr.root.poly.print();
          //curr.nodes[n].poly.print();
          Circuit newCirc = create_new(curr, oper, {curr.root, curr.nodes[n]}, n_models > 0);
          //newCirc.root.poly.print();

          int new_max = 0;
          for (auto const& [key, val] : newCirc.root.poly.poly_map) {
            if (abs(val) > 0) {
              new_max = abs(val);
              break;
            }
          }
          if (new_max == 0) continue;
          //cout << "hereee" << endl;
          /*cout << endl;
          cout << "new cost" << newCirc.cost << endl;
          cout << "max cost " << max_cost << endl;
          cout << "best cost " << best.cost << endl;
          cout << (newCirc.cost >= max_cost) << endl;
          cout << soln << endl;
          cout << (soln && newCirc.cost >= best.cost - 1) << endl;
          cout << (newCirc.cost >= max_cost || (soln && newCirc.cost >= best.cost - 1)) << endl;
          cout << !(newCirc.cost >= max_cost || (soln && newCirc.cost >= best.cost - 1)) << endl;*/
          if (newCirc.root.poly.poly_map == target.poly_map) {
            //cout << "best supposedly" << endl;
            //newCirc.root.poly.print();
            //return newCirc;
            //cout << "found solution" << endl;
            if (!soln || newCirc.cost < best.cost) {
              soln = true;
              best = newCirc;
              if (n_models > 0) {
                //cout << "updating models" << endl;
                PrioritizedCircuit pc = {newCirc.cost, newCirc, 0};
                potential_models.push_back(pc);
              }
            }
            solutions_found += 1;
          } else if (!(newCirc.cost >= max_cost || (soln && newCirc.cost >= best.cost - 1))) {
            float pred = get_pred(newCirc, false);
            //cout << "predicting " << pred << endl;
            if (pred < 1000000) {
              //cout << "PUSH BACKKKK " << pred << endl;
              //newCirc.root.poly.print();
              circs.push_back(newCirc);
              preds.push_back(pred);
              weights.push_back(1.0/pow(newCirc.cost - curr.cost + alpha * pred, gamma));

              float priority = newCirc.cost + alpha * pred; 
              if (wrapped) {
                priority = newCirc.cost + 1000 * get_pred(newCirc, true);
              }
              //cout << "n_models size" << n_models << endl;
              if (n_models > 0 && (models.size() < n_models || priority < models.back().priority)) {
                //cout << "pc push backkkk" << endl;
                PrioritizedCircuit pc = {priority, newCirc, pred};
                potential_models.push_back(pc);
              }
            }
          }
        }
      }
      if (potential_models.size() > 0 && n_models > 0) {
        //cout << "potential greater than zerooo" << endl;
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
        //cout << "blankkkk" << endl;
        vector<Circuit> modelCircuits = {};
        for (int k = 0; k < models.size(); k++) {
          modelCircuits.push_back(models[k].circuit);
        }
        curr = blank_circuit(modelCircuits);
      } else {
        random_device rd;
        mt19937 gen(rd());
        discrete_distribution<> d(weights.begin(), weights.end());
        
        int choice = d(gen);
        //cout << weights.size() << " choice " << choice << endl;
        curr = circs[choice];
        /*cout << "checking all nodes" << endl;
        for (int s = 0; s < circs.size(); s++) {
          cout << s << " " << circs[s].nodes.size() << endl;
        }
        cout << "node size after " << curr.nodes.size() << endl;*/
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
  poly = poly * poly;
  //poly.new_term({0, 0, 1}, 1);
  /*
  poly = poly * poly;
  //poly.new_term({0, 0, 1}, 1);
  //poly.new_term({0, 0, 2}, 2);
  Polynomial poly2 = Polynomial(3);
  poly2.new_term({0, 0, 1}, 2);
  poly = poly * poly2;
  poly = poly + poly2 * 2 + 2;
  */
  poly.print();


  /*Polynomial poly = Polynomial(3);
  poly.new_term({1, 0, 0}, 1);
  poly.new_term({0, 1, 0}, 1);
  //poly.new_term({0, 0, 1}, 1);
  poly = poly * poly;*/






  //return 0;
  Stochastic engine = Stochastic(poly, 8, {{add, 0.25}, {mult, 1}});
  Circuit sol = engine.sample_search(10000, 10, 1, 1, true, true, 3, true);

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

  //(int max_iters, int max_cost, float alpha, float gamma, bool verbose, bool use_pred, int n_models, bool wrapped) {

  /*Polynomial npoly = Polynomial();
  npoly = poly;


  set<vector<int>> set1 = {};
  set1.insert({1, 0, 0});
  set1.insert({1, 0, 1});
  set1.insert({1, 1, 1});


  set<vector<int>> set2 = {};
  set2.insert({1, 0, 2});
  set2.insert({1, 0, 1});
  set2.insert({1, 3, 1});

  set<vector<int>> set3;
  set<vector<int>> set4;
  set<vector<int>> set5;
  set_difference(set1.begin(), set1.end(), set2.begin(), set2.end(),
    inserter(set3, set3.end()));

  set_difference(set2.begin(), set2.end(), set1.begin(), set1.end(),
    inserter(set4, set4.end()));
  cout << set3.size() << endl;
  cout << set4.size() << endl;
  for (auto const& elem: set3) {
    for (auto const& e: elem) {
      cout << e << endl;
    }
  }
  set<vector<int>> set6;
  set_union(set3.begin(), set3.end(), set4.begin(), set4.end(),
    inserter(set5, set5.end()));
  cout << set5.size() << endl;
  set_difference(set1.begin(), set1.end(), set2.begin(), set2.end(),
    inserter(set6, set6.end()));
  cout << set6.size() << endl;*/
  //npoly.print();
  /*srand(time(0));
  cout << "hi1" << endl;
  cout << rand() << endl;
  cout << rand() << endl;
  cout << rand() << endl;
  Polynomial poly = Polynomial(3);
  poly.new_term({2, 0, 0}, 1);
  poly.new_term({0, 1, 1}, 1);
  poly.new_term({0, 0, 0}, 8);
  poly.print();

  Polynomial npoly = Polynomial(3);
  npoly.new_term({0, 2, 0}, 3);
  npoly.new_term({0, 1, 1}, 1);
  npoly.new_term({0, 0, 0}, 2);
  npoly.new_term({0, 0, 1}, 4);
  npoly.print();

  Polynomial poly1 = Polynomial(3);
  poly1.new_term({1, 0, 0}, 1);
  poly1.new_term({0, 1, 0}, 1);

  Polynomial poly2 = Polynomial(3);
  poly2.new_term({1, 0, 0}, 1);
  poly2.new_term({0, 1, 0}, 1);
  Polynomial mpoly = poly * poly;
  mpoly.print();*/
  return 0;
}
