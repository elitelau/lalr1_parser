CXX = g++
CFLAGS = -g -Wall -Winline -Wconversion -Woverloaded-virtual -Wshadow -Wcast-align -Wno-deprecated

CPP_SRC = $(wildcard *.cpp)
CPP_HD = $(wildcard *.h)
CPP_OBJ = $(subst .cpp,.o,${CPP_SRC})

INC = ../
LIB = ../
EXEC = lr_parser

${EXEC} : clean ${CPP_OBJ} $(LIB)/libminusc_parser.a
	$(CXX) *.o -o $@ -L$(LIB) -lminusc_parser

$(CPP_OBJ) : %.o : %.cpp $(CPP_HD)
	$(CXX) $(CFLAGS) -c -o $@ $< -I$(INC) 

clean:
	rm -f *.o ${EXEC}

