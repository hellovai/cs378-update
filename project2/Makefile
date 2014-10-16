FLAGS=-ffast-math -O3 -fno-tree-vectorize -lm
GPP=g++

all:
	$(GPP) part1.cpp fir.cpp -o part1 $(FLAGS)
	$(GPP) part2.cpp fir.cpp -o part2 $(FLAGS)

test1:
	./part1 sub040.data input/R.pcm input/L.pcm output/O_part1.pcm

test2:
	./part2 sub040.data 0 input/R.pcm output/O_part2.pcm

clean:
	rm *.pcm
	rm part1 part2
