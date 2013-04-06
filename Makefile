run: tsp.h tsp.cpp application.cpp graph.cpp
	mpic++ -o run application.cpp tsp.cpp graph.cpp -lm `pkg-config --cflags --libs opencv`
