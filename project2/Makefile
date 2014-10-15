FLAGS=-ffast-math -O3 -fno-tree-vectorize

all:
	g++ part1.cpp fir.cpp -o part1 $(FLAGS)
	g++ part2.cpp fir.cpp -o part2 $(FLAGS)

test1:
	./part1 sub040.data samples/01.R.pcm samples/01.L.pcm R.pcm L.pcm

test2:
	./part2 sub040.data -80 0 samples/01.R.pcm R_m.pcm L_m.pcm
