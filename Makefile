CXX:=$(shell sh -c 'type $(CXX) >/dev/null 2>/dev/null && echo $(CXX) || echo g++')
#CXX = clang++

MYSQL_C_INC ?= $(shell mysql_config --cflags)
MYSQL_C_LIBS ?= $(shell mysql_config --libs)

GMIME_INCLUDES ?=  $(shell pkg-config gmime-2.6 --cflags)
GMIME_LIBS ?=  $(shell pkg-config gmime-2.6 --libs)

CLD_INCLUDES = -I/usr/local/include/cld
CLD_LIBS = -lcld2

OPTIMIZATION = -O0

WARNINGS =     \
                -Wall \
                -Wno-sign-compare \
                -Wno-deprecated-register \
                -Wno-unused-function

DEBUG = -g

STDC = -std=c++1y

LDFLAGS = -lstdc++ -pthread

CXXFLAGS = $(OPTIMIZATION) -fPIC -fstack-protector $(CFLAGS) $(WARNINGS) $(DEBUG) $(STDC) -DUSE_CXX0X

STDLIB =
ifeq ($(shell uname -s), FreeBSD)
STDLIB +=  -stdlib=libc++
LIBXML_INCLUDES =
LIBXML_INCLUDES = -I/usr/local/include/libxml2
endif
CXXFLAGS +=  $(STDLIB)

CFLAGS=-c -Wall

LIB_SOURCES = src/client.cpp \
          src/conn_pool.cpp \
          src/pipeline.cpp \
          src/serial.cpp \
          src/proto.cpp \
          src/query.cpp \
          src/io_buffers.cpp \
          src/buff_abstract.cpp \
	  src/cmd_traits.cpp \
          utils/h_net.cpp \
          utils/h_strings.cpp \

LIB_NAME = asio-redis.so

LIB_OBJECTS=$(SOURCES:.cpp=.o)

EXAMPLES_SOURCES = examples/direct_buff.cpp \
                   src/client.cpp \
                   src/conn_pool.cpp \
                   src/pipeline.cpp \
                   src/proto.cpp \
                   src/query.cpp \
                   src/io_buffers.cpp \
                   src/buff_abstract.cpp \
		   src/cmd_traits.cpp \
                   src/serial.cpp \
                   utils/h_net.cpp \
                   utils/h_strings.cpp

EXAMPLES_EXEC = asio-redis-ex

EXAMPLES_OBJ=$(EXAMPLES_SOURCES:.cpp=.o)

all: $(EXAMPLES_SOURCES) $(EXAMPLES_EXEC)


$(EXAMPLES_EXEC): $(EXAMPLES_OBJ)
	$(CXX) $(LDFLAGS) $(EXAMPLES_OBJ) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean: 
	rm $(LIB_OBJECTS) $(LIB_NAME) $(EXAMPLES_OBJ) $(EXAMPLES_EXEC)

