# Parallel Programming Assignment

This repository is based on the code written by Davide Gadioli, T.A. of _Advanced Algorithms and Parallel Programming_ course.
The main focus of the assignment was to parallelize a serial code that computes the coverage for some molecules, using either MPI or OpenMP.

I personally used _MPI_.

## Repository structure

At the root level, we have the main `CMakeLists.txt` file that drives the compilation phases.
The actual source files are stored in the folder `src`.
There are some sample files with a few molecules in `data/molecules.smi`.
In `scripts/launch.sh` is present a script that wraps the execution to set a fixed level of parallelism.


## How to compile the application

Assuming that the working directory is the repository root, it is enough to issue the following commands:

```bash
$ cmake -S . -B build
$ cmake --build build
```

## How to run the application

You can find the executable in the building directory.
For example, if the build directory is `build`, the executable will be `./build/main`.
The executable reads the input molecules from the standard input, writes information about the execution on the standard error, and prints the final table on the standard output.

For example, assuming that the working directory is in the repository root, and the building directory is `./build`, then, you can execute the application with the script:

```bash
$ ./scripts/launch.sh ./build/main ./data/molecules.smi output.csv 1
```

You will see some information on the terminal, while the final output is stored in the output.csv file.

> **NOTE**: you can find more datasets with a larger number of molecules here: [https://github.com/GLambard/Molecules_Dataset_Collection/tree/master](https://github.com/GLambard/Molecules_Dataset_Collection/tree/master)

To be able to use datasets downloaded from that github repository, you can run the python script `extract_smile.py`, modifying the code based on the file. Finally, you have to convert the resulting ".smi" file using the command `dos2unix`.

There are some additional scripts, `check_output.py` to check the correctness of the output file, compared with the one obtained by the serial implementation, and `evaluate_time.py` to run multiple executions of the program, obtaining the scale factor incresing the number of processors.
