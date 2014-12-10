#!/bin/bash

SIZE=(1000)
THREAD=(16 32)
ITERATIONS=(200)
DENSITY=(0.6 0.7 0.8 0.9 1.0)

g++ jacobi.cpp -std=c++0x -o jacobi.out -fopenmp

for s in "${SIZE[@]}"; do
  for t in "${THREAD[@]}"; do
    for i in "${ITERATIONS[@]}"; do
      for d in "${DENSITY[@]}"; do
        ./jacobi.out $s $i $d $t
      done
    done
  done
done
