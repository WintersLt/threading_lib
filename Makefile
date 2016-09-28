CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS= -L . -L../ ./mythread.a
SOURCES=./src/list.c ./src/mythread.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=hello

INCLUDES=-I ./inc

all: $(SOURCES) $(OBJECTS) mythread_a extra_testcase_joinall extra_testcase_yield testcase_joinall testcase_passing testcase_semaphore
#$(EXECUTABLE): $(OBJECTS) 
#    $(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
mythread_a:
	ar crf mythread.a ./src/*.o

####test_semaphore: 
####	rm -f tests/testing_info/test_semaphore
####	$(CC) $(INCLUDES) tests/testing_info/TestcasePrograms/test_semaphore.c -o $@ $(LDFLAGS)
####	mv test_semaphore tests/testing_info

extra_testcase_joinall: 
	rm -f tests/testing_info/extra_testcase_joinall
	$(CC) $(INCLUDES) tests/testing_info/TestcasePrograms/extra_testcase_joinall.c -o $@ $(LDFLAGS)
	mv extra_testcase_joinall tests/testing_info

extra_testcase_yield: 
	rm -f tests/testing_info/extra_testcase_yield
	$(CC) $(INCLUDES) tests/testing_info/TestcasePrograms/extra_testcase_yield.c -o $@ $(LDFLAGS)
	mv extra_testcase_yield tests/testing_info

testcase_joinall: 
	rm -f tests/testing_info/testcase_joinall
	$(CC) $(INCLUDES) tests/testing_info/TestcasePrograms/testcase_joinall.c -o $@ $(LDFLAGS)
	mv testcase_joinall tests/testing_info

testcase_semaphore: 
	rm -f tests/testing_info/testcase_semaphore
	$(CC) $(INCLUDES) tests/testing_info/TestcasePrograms/testcase_semaphore.c -o $@ $(LDFLAGS)
	mv testcase_semaphore tests/testing_info

testcase_passing: 
	rm -f tests/testing_info/testcase_passing
	$(CC) $(INCLUDES) tests/testing_info/TestcasePrograms/testcase_passing.c -o $@ $(LDFLAGS)
	mv testcase_passing tests/testing_info

####test_mythread1: 
####	rm -f tests/test_mythread1
####	$(CC) $(INCLUDES) tests/TestMyThread1.c -o $@ $(LDFLAGS)
####	mv test_mythread1 tests

####test_passing: 
####	rm -f tests/test_passing
####	$(CC) $(INCLUDES) tests/passing.c -o $@ $(LDFLAGS)
####	mv test_passing tests

####test_ping: 
####	rm -f tests/test_ping
####	$(CC) $(INCLUDES) tests/ping.c -o $@ $(LDFLAGS)
####	mv test_ping tests

####test_tree: 
####	rm -f tests/test_tree
####	$(CC) $(INCLUDES) tests/tree.c -o $@ $(LDFLAGS)
####	mv test_tree tests

####sem_test: 
####	rm -f tests/sem_test
####	$(CC) $(INCLUDES) tests/sem_test.c -o $@ $(LDFLAGS)
####	mv sem_test tests

####sem_premature_destroy: 
####	rm -f tests/sem_premature_destroy
####	$(CC) $(INCLUDES) tests/sem_premature_destroy.c -o $@ $(LDFLAGS)
####	mv sem_premature_destroy tests

####test_extra_test: 
####	rm -f tests/test_extra_test
####	$(CC) $(INCLUDES) tests/extra_test.c -o $@ $(LDFLAGS)
####	mv test_extra_test tests

####test_mythreadextra: 
####	rm -f tests/test_mythreadextra
####	$(CC) $(INCLUDES) tests/TestMyThreadExtra.c -o $@ $(LDFLAGS)
####	mv test_mythreadextra tests


clean:
	rm -f ./src/*.o ./src/*.a
	rm -f ./tests/*.o ./tests/*.a
