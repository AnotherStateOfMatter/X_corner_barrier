#tool macros
CXX := gcc
CXXFLAGS := -O2

pointer_barrier: main.o
	g++ main.o -o pointer_barrier $(CXXFLAGS) -lX11 -lXfixes -lX11 -lXrandr -lXi
	rm *.o

main.o: X11_corner_barrier.c
	$(CXX) -c $(CXXFLAGS) X11_corner_barrier.c -o main.o

.PHONY: clean
clean:
	rm mouse_corner_barrier