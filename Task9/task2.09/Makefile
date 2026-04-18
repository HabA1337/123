CXX = g++
CXXFLAGS = -std=c++17 -O3 -mfpmath=sse -fstack-protector-all -g -W -Wall -Wextra \
           -Wunused -Wcast-align -Werror -pedantic -pedantic-errors \
           -Wfloat-equal -Wpointer-arith -Wformat-security \
           -Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align \
           -Wno-long-long -Woverloaded-virtual -Wnon-virtual-dtor -Wcast-qual \
           -Wno-suggest-attribute=format

SERVER_SRCS = server.cpp command.cpp record.cpp
CLIENT_SRCS = client.cpp

SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

all: server client

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) server client

server.o: server.cpp record.h command.h record_list.h name_index.h phone_index.h group_index.h dyn_array.h
client.o: client.cpp
command.o: command.cpp command.h record.h
record.o: record.cpp record.h
