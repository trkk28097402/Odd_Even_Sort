#!/bin/bash

#SBATCH -J oddeven.log         # Job Name
#SBATCH -A ACD110018           # Account
#SBATCH -p ctest               # Partition
#SBATCH -o oddeven_out.log     # Redirect `stdout` to File
#SBATCH -e oddeven_err.log     # Redirect `stderr` to File

#SBATCH -n 12
#SBATCH -N 3

module purge
module load compiler/intel/2022
module load IntelMPI/2021.6

export UCX_NET_DEVICES=all

time mpirun ./odd_even_sort 536869888 ./testcases/40.in out.txt