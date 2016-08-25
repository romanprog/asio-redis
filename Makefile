CXX:=$(shell sh -c 'type $(CXX) >/dev/null 2>/dev/null && echo $(CXX) || echo g++')

INSTALL = install

INSTALL_INCLUDE_PATH = /usr/local/include
INSTALL_LIB_PATH = /usr/local/lib

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
          src/h_net.cpp \
          src/h_strings.cpp \

LIB_NAME = libasioredis.so

LIB_OBJECTS=$(LIB_SOURCES:.cpp=.o)

EX_BASE_SOURCES = examples/example.cpp

EX_BASE_EXEC = base-exampe

EX_BASE_OBJ=$(EX_BASE_SOURCES:.cpp=.o)

EX_DBUF_SOURCES = examples/direct_buff.cpp

EX_DBUF_EXEC = direct-buf-example

EX_DBUF_OBJ=$(EX_DBUF_SOURCES:.cpp=.o)


all: $(LIB_SOURCES) $(LIB_NAME)

examples: $(EX_BASE_SOURCES) $(EX_BASE_EXEC) $(EX_DBUF_SOURCES) $(EX_DBUF_EXEC)

install: $(LIB_NAME)
	$(INSTALL) $(LIB_NAME) $(INSTALL_LIB_PATH)
	$(INSTALL) asio-redis.hpp $(INSTALL_INCLUDE_PATH)
	mkdir -p $(INSTALL_INCLUDE_PATH)/asio-redis
	cp -r include/* $(INSTALL_INCLUDE_PATH)/asio-redis/

reinstall: $(LIB_NAME)
	  rm -f $(INSTALL_LIB_PATH)/$(LIB_NAME)
	  rm -rf $(INSTALL_INCLUDE_PATH)/asio-redis/
	  $(INSTALL) $(LIB_NAME) $(INSTALL_LIB_PATH)
	  $(INSTALL) asio-redis.hpp $(INSTALL_INCLUDE_PATH)
	  mkdir -p $(INSTALL_INCLUDE_PATH)/asio-redis
	  cp -r include/* $(INSTALL_INCLUDE_PATH)/asio-redis/


$(EX_BASE_EXEC): $(EX_BASE_OBJ)
	$(CXX)  $(LDFLAGS) $(EX_BASE_OBJ) -o $@ -lasioredis

$(EX_DBUF_EXEC): $(EX_DBUF_OBJ)
	$(CXX)  $(LDFLAGS) $(EX_DBUF_OBJ) -o $@ -lasioredis

$(LIB_NAME): $(LIB_OBJECTS)
	$(CXX) -fPIC -shared $(LDFLAGS) $(LIB_OBJECTS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean: 
	rm -f $(LIB_OBJECTS) $(LIB_NAME) $(EX_BASE_OBJ) $(EX_DBUF_OBJ) $(EX_BASE_EXEC) $(EX_DBUF_EXEC)

