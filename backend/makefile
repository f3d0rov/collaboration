
CXX = clang++
CC = clang
CXX_FLAGS = -ggdb -std=c++20 -O3 
C_FLAGS=

ODIR = obj
SOURCE_DIR = source
INCLUDE = 
LIB_INCLUDE = 
LIBS = -lpthread -static-libgcc -ldl -static-libstdc++ -lpqxx -lpq -lcivetweb -lcrypto

OUT = build/collab-server


CPP_SRCS = $(wildcard $(SOURCE_DIR)/*.cpp $(SOURCE_DIR)/*/*.cpp)
OBJS = $(patsubst $(SOURCE_DIR)%,$(ODIR)%,$(patsubst %.cpp,%.o,$(CPP_SRCS)))


all: $(OBJS)
	mkdir -p build
	$(CXX) -o $(OUT) $^ $(CXX_FLAGS) $(LIB_INCLUDE) $(LIBS)

$(ODIR)/%.o: $(SOURCE_DIR)/%.cpp
	mkdir -p $(dir $(patsubst $(SOURCE_DIR)%,$(ODIR)%,$<))
	$(CXX) -c -o $@ $< $(CXX_FLAGS) $(INCLUDE)

clear:
	rm -r $(ODIR)/*

clean: clear

.PHONY: clean

	
