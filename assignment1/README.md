# Advanced Algorithms Assignments

## Base case
The code in `main.cpp` solves the famous "alignment problem" in biology: given 2 strings composed by characters "A", "T", "G", "C" (Adenine, Thymine, Guanine, Cytosine), 
the goal is to align those two sequences by inserting gaps or admitting differences paying the minimum cost.
- Any time you have a gap, an ‘\_’ is inserted in one of the sequences. Any ‘\_’ adds a cost of 2 units to the final solution
- Any time you allow a difference, replace the two characters with a ‘\*’. Any ‘\*’ adds a cost of 5 units to the final solution (10 on both strings)

If you want to try the code use the following commands:


`g++ main.cpp alignment.cpp -o alignment`

`./alignment`


## Bonus case
The code in `main_bonus.cpp` solves a problem related to the previous one. 
Given two numbers n and m:
- Generate two strings composed of "A", "T", "G", "C" characters that produce the maximum cost
- The gap and the replacement costs are parametric

If you want to try the code use the following commands:

`g++ main_bonus.cpp alignment.cpp -o alignment_bonus`

`./alignment_bonus`
