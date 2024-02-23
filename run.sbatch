#!/bin/bash
#SBATCH -JlibRun
#SBATCH -A [Account]
#SBATCH -N1 --ntasks-per-node=4
#SBATCH -t1
#SBATCH -qinferno
#SBATCH -oReport-%j.out

module load intel/20.0.4
module load intel-tbb/2020.3
./build/src/lz_library