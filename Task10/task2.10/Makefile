CXX = g++
CXXFLAGS = -std=c++17 -O3 -mfpmath=sse -fstack-protector-all -g -W -Wall -Wextra \
           -Wunused -Wcast-align -Werror -pedantic -pedantic-errors -Wfloat-equal \
           -Wpointer-arith -Wformat-security -Wmissing-format-attribute -Wformat=1 \
           -Wwrite-strings -Wno-long-long -Woverloaded-virtual -Wnon-virtual-dtor \
           -Wcast-qual -Wno-suggest-attribute=format

ALL = a01.out a02.out a03.out a04.out a05.out a06.out

all: $(ALL)

a01.out: a01.cpp io_status.h
	$(CXX) $(CXXFLAGS) a01.cpp -o $@

a02.out: a02.cpp io_status.h
	$(CXX) $(CXXFLAGS) a02.cpp -o $@

a03.out: a03.cpp io_status.h
	$(CXX) $(CXXFLAGS) a03.cpp -o $@

a04.out: a04.cpp io_status.h
	$(CXX) $(CXXFLAGS) a04.cpp -o $@

a05.out: a05.cpp io_status.h
	$(CXX) $(CXXFLAGS) a05.cpp -o $@

a06.out: a06.cpp io_status.h
	$(CXX) $(CXXFLAGS) a06.cpp -o $@

clean:
	rm -f $(ALL)
