#include "alignment.hpp"
using namespace std;

int main(){
    // Ask for the 2 strings
    string X, Y;
    cout << "Y: ";
    cin >> Y;
    cout << "X: ";
    cin >> X;
    int gap = 2;
    int sub = 5;

    pair<pair<string, string>, int> sol = align_min_cost(X, Y, gap, sub);
    cout << "Aligned string Y: " << sol.first.first << endl;
    cout << "Aligned string X: " << sol.first.second << endl;
    cout << "Cost: " << sol.second << endl;

    return 0;
}