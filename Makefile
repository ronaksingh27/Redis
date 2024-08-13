CXX = g++

CXXFLAGs = -Wall -Wextra -O2 -g

SOURCES = client.cpp server.cpp

OBJECTS = $(SOURCES: .cpp = .o )

EXEC = server client

all : $(EXEC)


server : server.o
	$(CXX) $(CXXFLAGS) -o server server.o

client : client.o
	$(CXX) $(CXXFLAGS) -o client client.o

clean :
	rm -f *.o $(EXEC)
