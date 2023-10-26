#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <iostream>
#include <vector>
using namespace std;

pair<pair<string, string>, int> align_min_cost(string X, string Y, int gap, int sub);
pair<string, string> build_max_cost(int n, int m, int gap, int sub);
pair<string, string> build_max_disjoint(int n, int m, int gap, int sub);
void print_memo(vector<vector<int>> memo, string X, string Y);

#endif