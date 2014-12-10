all: jacobi

jacobi:
	g++ jacobi.cpp -std=c++0x -o jacobi.out -pthread

clean:
	rm -rf *.out

simple:
	./jacobi.out 10 1 0.7 1
test:
	./jacobi.out 10 1 0.7 4
	./jacobi.out 10 10 0.7 4
	./jacobi.out 10 100 0.7 4
	./jacobi.out 10 1000 0.7 4
	./jacobi.out 10 10000 0.7 4
