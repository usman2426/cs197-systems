#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
using namespace std;

// taken from https://jimmy-shen.medium.com/stl-map-unordered-map-with-a-vector-for-the-key-f30e5f670bae
struct VectorHasher {
    int operator()(const vector<int> &V) const {
        int hash = V.size();
        for(auto &i : V) {
            hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

const string letters = "abcdefghijklmnopqrstuvwxyz";

class Polynomial {
  public:
  int n_var;
  //map<vector<int>, int> poly_map;
  unordered_map<vector<int>, int, VectorHasher> poly_map;

  Polynomial() {
    n_var = 0;
    poly_map = {};
  }

  Polynomial(int n) {
    n_var = n;
    poly_map = {};
  }

  void new_term(vector<int> powers, int coeff) {
    poly_map[powers] = coeff;
  }

  void print() {
    string output = "";
    for (auto const& [key, val] : poly_map) {
      if (val == 0) continue;
      output += to_string(val);
      
      for (int i = 0; i < n_var; i++) {
        if (key[i] == 0) continue;
        output.append(1, letters[i]);

        if (key[i] > 1) {
          output += "^" + to_string(key[i]);
        }
        
      }
      output += " + ";

    }
    cout << output.substr(0, output.size() - 3) << endl; // -3 to get rid of the extra plus sign and spaces at the end
  }

  const Polynomial& operator + (const Polynomial &obj) const {
    static Polynomial result = Polynomial(n_var);
    result.poly_map = poly_map;

    for (auto const& [key, val] : obj.poly_map) {
      if (result.poly_map.count(key)) {
        //result.poly_map[key] += obj.poly_map[key];
        result.poly_map.at(key) += obj.poly_map.at(key);
      } else {
        //result.poly_map[key] = obj.poly_map[key];
        result.poly_map.insert({key, obj.poly_map.at(key)});
      }
    }
    return result;
  }

  const Polynomial& operator - (const Polynomial &obj) const {
    static Polynomial result = Polynomial(n_var);
    result.poly_map = poly_map;

    for (auto const& [key, val] : obj.poly_map) {
      if (result.poly_map.count(key)) {
        //result.poly_map[key] -= obj.poly_map[key];
        result.poly_map.at(key) -= obj.poly_map.at(key);
      } else {
        //result.poly_map[key] = obj.poly_map[key] * -1;
        result.poly_map.insert({key, obj.poly_map.at(key) * -1});
      }
    }
    return result;
  }

  const Polynomial operator * (const Polynomial &obj) const {
    Polynomial result = Polynomial(n_var);
    result.poly_map = {};

    for (auto const& [key1, val1] : poly_map) {
      for (auto const& [key2, val2] : obj.poly_map) {
        vector<int> new_pow = {};
        for (int i = 0; i < n_var; i++) {
          new_pow.push_back(key1[i] + key2[i]);
        }
        if (result.poly_map.count(new_pow)) {
          result.poly_map[new_pow] += val1 * val2;
        } else {
          result.poly_map[new_pow] = val1 * val2;
        }
      }
    }
    return result;
  }

  const Polynomial operator * (const int &constant) const {
    Polynomial result = Polynomial(n_var);
    result.poly_map = {};

    for (auto const& [key, val] : poly_map) {
      result.poly_map[key] = val * constant;
    }
    return result;
  }

  const Polynomial operator + (const int &constant) const {
    Polynomial result = Polynomial(n_var);
    result.poly_map = {};

    for (auto const& [key, val] : poly_map) {
      result.poly_map[key] = val;
    }
    vector<int> term(n_var, 0);
    result.poly_map[term] = constant;
    
    return result;
  }

};


