all: main

main:
	g++ -Wall main.cc serve.cc socket.cc http.cc eventLoop.cc file_system.cc -o main
