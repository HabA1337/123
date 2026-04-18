CXX = g++
CXXFLAGS = -std=c++17 -O3 -mfpmath=sse -fstack-protector-all -g -W -Wall -Wextra \
           -Wunused -Wcast-align -Werror -pedantic -pedantic-errors \
           -Wfloat-equal -Wpointer-arith -Wformat-security \
           -Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align \
           -Wno-long-long -Woverloaded-virtual -Wnon-virtual-dtor -Wcast-qual \
           -Wno-suggest-attribute=format

SRCS = a.cpp command.cpp record.cpp
OBJS = $(SRCS:.cpp=.o)
TARGETS = a.out

all: $(TARGETS)

%.out: %.o command.o record.o
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGETS)

a.o: a.cpp record.h command.h record_list.h name_index.h phone_index.h dyn_array.h
command.o: command.cpp command.h record.h
record.o: record.cpp record.h
