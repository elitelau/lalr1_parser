CXX = /opt/centos/devtoolset-1.1/root/usr/bin/c++
CXX = g++
CFLAGS = -g -Wall -Winline -Wconversion -Woverloaded-virtual -Wshadow -Wcast-align -Wno-deprecated

CPP_SRC = $(wildcard *.cpp)
CPP_HD = $(wildcard *.h)
CPP_OBJ = $(subst .cpp,.o,${CPP_SRC})

LIB = libminusc_parser.a

${LIB} : ${CPP_OBJ}
	ar -rvs $@ *.o

$(CPP_OBJ) : %.o : %.cpp $(CPP_HD)
	$(CXX) $(CFLAGS) -c -o $@ $< 

clean:
	rm -f *.o tables.txt ${LIB}

