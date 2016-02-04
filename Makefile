CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS= -L . -L../ ./mythread.a
SOURCES=./src/list.c ./src/mythread.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=hello

INCLUDES=-I ./inc

all: $(SOURCES) $(OBJECTS) mythread_a test_mythread1 test_passing test_ping test_tree sem_test sem_premature_destroy test_mythreadextra
    
#$(EXECUTABLE): $(OBJECTS) 
#    $(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
mythread_a:
	ar crf mythread.a ./src/*.o

test_mythread1: 
	rm -f tests/test_mythread1
	$(CC) $(INCLUDES) tests/TestMyThread1.c -o $@ $(LDFLAGS)
	mv test_mythread1 tests

test_passing: 
	rm -f tests/test_passing
	$(CC) $(INCLUDES) tests/passing.c -o $@ $(LDFLAGS)
	mv test_passing tests

test_ping: 
	rm -f tests/test_ping
	$(CC) $(INCLUDES) tests/ping.c -o $@ $(LDFLAGS)
	mv test_ping tests

test_tree: 
	rm -f tests/test_tree
	$(CC) $(INCLUDES) tests/tree.c -o $@ $(LDFLAGS)
	mv test_tree tests

sem_test: 
	rm -f tests/sem_test
	$(CC) $(INCLUDES) tests/sem_test.c -o $@ $(LDFLAGS)
	mv sem_test tests

sem_premature_destroy: 
	rm -f tests/sem_premature_destroy
	$(CC) $(INCLUDES) tests/sem_premature_destroy.c -o $@ $(LDFLAGS)
	mv sem_premature_destroy tests

test_mythreadextra: 
	rm -f tests/test_mythreadextra
	$(CC) $(INCLUDES) tests/TestMyThreadExtra.c -o $@ $(LDFLAGS)
	mv test_mythreadextra tests

clean:
	rm -f ./src/*.o ./src/*.a
	rm -f ./tests/*.o ./tests/*.a
