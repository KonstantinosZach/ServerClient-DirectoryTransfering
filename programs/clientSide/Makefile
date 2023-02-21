# A SIMPY MAKEFILE FOR C++ PROGRAMS

# paths
INCLUDE = ../../includes
MODULES = ../../modules

#compiler
CC = g++

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -Wall  -g -I$(INCLUDE) -std=c++11

# Αρχεία .o
OBJS =  remoteClient.o $(MODULES)/client.o

# Το εκτελέσιμο πρόγραμμα
EXEC = remoteClient 

# Command line arguments
ARGS = -i 195.134.65.75 -p 12501 -d ../../dumps/

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

clean:
	rm -f $(OBJS) $(EXEC) \
	rm -r ../../serverOutput/*

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind:
	valgrind --leak-check=full ./$(EXEC) $(ARGS)