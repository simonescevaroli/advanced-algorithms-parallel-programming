#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <algorithm>
using namespace std;


void print_memo(vector<vector<int>> memo, string X, string Y);
void align_min_cost(string X, string Y);

int main()
{
    // Ask for the 2 strings
    string X, Y;
    cout << "X: ";
    cin >> X;
    cout << "Y: ";
    cin >> Y;

    align_min_cost(X, Y);

  return 0;

}

void align_min_cost(string X, string Y){
    // The X string is displayed in the columns, while the Y string is displayed in the rows
    int m = X.length();
    int n = Y.length();
    int gap = 2;
    int sub = 5;

    vector<vector<int>> memo(n + 1, vector<int>(m + 1, 0));

    // First row and first column initialized with gap penalty times the number of letters considered
    // (Base case: one string is empty, the other is not, the only thing you can do is adding gaps)
    for(int i = 0; i <= m; i++){
        memo[0][i] = i * gap;
    }
    for(int i = 0; i <= n; i++){
        memo[i][0] = i * gap;
    }

    for(int i = 1; i <= n; i++){
        for(int j = 1; j <= m; j++){
            if(X[j-1] == Y[i-1]){
                // The character is the same, no additional cost w.r.t. the previous one
                memo[i][j] = memo[i-1][j-1];
            } else {
                // Store the minimum cost computed as the subproblem minimum cost plus:
                // The substitution of both the character (*) -> cost 2*sub
                // The insertion of a gap on string X -> cost gap
                // The insertion of a gap on string Y -> cost gap
                memo[i][j] = min({memo[i-1][j-1] + 2 * sub, memo[i-1][j] + gap, memo[i][j-1] + gap});
            }
        }
    }

    // Reconstructe the solution starting from the cost table
    int i = n;  //index for row
    int j = m;  //index for column
    vector<char> aligned_X;
    vector<char> aligned_Y;

    // This solution always prefers first gaps on the X string
    while (i != 0 && j != 0){
        if(X[j-1] == Y[i-1]){
            //same character
            aligned_X.push_back(X[j-1]);
            aligned_Y.push_back(Y[i-1]);
            i--;
            j--;
        }else if(memo[i][j] - 2 * sub == memo[i-1][j-1]){
            // If the choice was the "*"
            aligned_X.push_back('*');
            aligned_Y.push_back('*');
            i--;
            j--;
        }else if(memo[i][j] - gap == memo[i-1][j]){
            // If the choice was the "_" in the X string
            aligned_X.push_back('_');
            aligned_Y.push_back(Y[i-1]);            
            i--;
        }else if(memo[i][j] - gap == memo[i][j-1]){
            // If the choice was the "_" in the Y string
            aligned_X.push_back(X[j-1]);
            aligned_Y.push_back('_');            
            j--;
        }
    }

    //If one of the two strings ends, clear the other one adding "_" in the empty one
    while(i!=0){
        // String X is empty
        aligned_X.push_back('_');
        aligned_Y.push_back(Y[i-1]);          
        i--;
    }
    while(j!=0){
        // String Y is empty
        aligned_X.push_back(X[j-1]);
        aligned_Y.push_back('_');            
        j--;
    }

    cout << "Aligned string X: ";
    for(auto it = aligned_X.rbegin(); it != aligned_X.rend(); ++it){
        cout << *it;
    }
    cout << endl << "Aligned string Y: ";
    for(auto it = aligned_Y.rbegin(); it != aligned_Y.rend(); ++it){
        cout << *it;
    }
    cout << endl << "Cost: " << memo[n][m] << endl;
}

void print_memo(vector<vector<int>> memo, string X, string Y){
    cout << "    ";
    for(auto letter : X){
        cout << letter << " ";
    }
    cout << endl;
    for(int i = 0; i <= Y.length(); i++){
        if(i==0){
            cout << "  ";
        }else{
            cout << Y[i-1] << " ";
        }
        for(int j = 0; j <= X.length(); j++){
            cout << memo[i][j] << " ";
        }
            cout << endl;
    }
}