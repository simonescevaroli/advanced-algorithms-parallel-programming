#include "alignment.hpp"
using namespace std;

int main(){
    // Ask for the 2 strings
    string X, Y;
    int n, m, gap, sub;
    cout << "n: ";
    cin >> n;
    cout << "m: ";
    cin >> m;
    cout << "Cost for gap: ";
    cin >> gap;
    cout << "Cost for replacement: ";
    cin >> sub;
    
    pair<string, string> sol = build_max_cost(n, m, gap, sub);
    //pair<string, string> sol = build_max_disjoint(n, m, gap, sub);

    cout << "Y: " << sol.first << endl;
    cout << "X: " << sol.second << endl;
    cout << "---------------------" << endl;
    pair<pair<string, string>, int> p = align_min_cost(sol.first, sol.second, gap, sub);
    cout << "Aligned Y: " << p.first.first << endl;
    cout << "Aligned X: " << p.first.second << endl;
    cout << "Max cost is: " << p.second << endl;
}