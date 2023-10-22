#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>
using namespace std;

#define MAX_L 100

void print_memo(vector<vector<int>> memo, string X, string Y);

int main()
{
  // Ask for the 2 strings
  string X, Y;
  cout << "First string X: ";
  cin >> X;
  cout << "Second string Y: ";
  cin >> Y;

  int m = X.length();
  int n = Y.length();

  // Define the matrix for memoization and initialize it with all 0s
  vector<vector<int>> memo(m, vector<int>(n));

  return 0;

}

void print_memo(vector<vector<int>> memo, string X, string Y)
{
    cout << "  ";
    for(auto letter : Y)
    {
        cout << letter << " ";
    }
    cout << endl;
    for(int i = 0; i < X.length(); i++)
    {
        cout << X[i] << " ";
        for(int j = 0; j < Y.length(); j++)
        {
            cout << memo[i][j] << " ";
        }
            cout << endl;
    }
}