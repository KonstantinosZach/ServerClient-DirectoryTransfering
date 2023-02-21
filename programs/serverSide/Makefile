# A SIMPY MAKEFILE FOR C++ PROGRAMS

# paths
INCLUDE = ../../includes
MODULES = ../../modules

#compiler
CC = g++

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -Wall  -g -I$(INCLUDE) -std=c++11

# Αρχεία .o
OBJS =  dataServer.o $(MODULES)/server.o

# Το εκτελέσιμο πρόγραμμα
EXEC = dataServer 

# Command line arguments
ARGS = -p 12501 -s 5 -q 3 -b 128

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -lpthread -o $(EXEC)

clean:
	rm -f $(OBJS) $(EXEC) \

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind:
	valgrind --leak-check=full ./$(EXEC) $(ARGS)