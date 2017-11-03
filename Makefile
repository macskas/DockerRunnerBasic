CXX=g++
CXXFLAGS=-march=native -Wall -O2 -I /usr/local/include/ -std=c++0x
LDFLAGS=-L/usr/local/lib
LIBS=-ldl -lrt
DOCKERRUNNER=docker-runner
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: $(DOCKERRUNNER)
static: $(DOCKERRUNNER).static

$(DOCKERRUNNER): $(OBJS)
                $(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(DOCKERRUNNER).static: $(OBJS)
                $(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS) -static

%.o: %.cpp %.h
                $(CXX) -c $(CXXFLAGS) -o $@ $<
clean:
                rm -f $(DOCKERRUNNER) $(OBJS)
