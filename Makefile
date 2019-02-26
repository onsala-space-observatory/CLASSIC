CC            = gcc
CXX           = g++
DEFINES       =
INCPATH       =
CFLAGS        = -Wno-unused-parameter -fstack-protector-all -g -Wall -W -D_REENTRANT -fPIC $(DEFINES)
CXXFLAGS      = -Wno-unused-parameter -fstack-protector-all -g -Wall -W -D_REENTRANT -fPIC $(DEFINES)
RM            = rm -f
PYTHON        = python3

OBJECTS       = main.o \
		class.o

classictest: main.o class.o
	$(CXX) -o classictest main.o class.o -lm

main.o: main.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o main.o main.cpp

class.o: class.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o class.o class.cpp

clean: clean-build clean-pyc
	-$(RM) $(OBJECTS)

clean-build:
	rm -fr build/
	rm -fr dist/
	rm -fr .eggs/
	find . -name '*.egg-info' -exec rm -fr {} +
	find . -name '*.egg' -exec rm -f {} +

clean-pyc:
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	find . -name '*~' -exec rm -f {} +
	find . -name '__pycache__' -exec rm -fr {} +

install: clean
	$(PYTHON) setup.py install --user
