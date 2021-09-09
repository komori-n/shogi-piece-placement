TARGET  = ./shogi-piece-placement.out
TESTTARGET = ./gtest.out

CC      = g++ -O3 -std=c++17 -fopenmp -pthread -mavx2
CFLAGS  = -Wall -MMD -MP
GTEST_DIR = /usr/local

SRC_DIR = ./src
OBJ_DIR = ./obj
MAINSRC = $(SRC_DIR)/main.cpp
SOURCES = $(shell ls $(SRC_DIR)/*.cpp | grep -v $(MAINSRC))
MAINOBJ = $(OBJ_DIR)/main.o
OBJS    = $(subst $(SRC_DIR),$(OBJ_DIR), $(SOURCES:.cpp=.o))

TEST_DIR= ./test
TESTOBJ_DIR = ./testobj
TESTSRC = $(shell ls $(TEST_DIR)/*.cpp)
TESTOBJ = $(subst $(TEST_DIR), $(TESTOBJ_DIR), $(TESTSRC:.cpp=.o))
# GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
#                 $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_LIBS = $(GTEST_DIR)/lib/libgtest.a $(GTEST_DIR)/lib/libgtest_main.a

DEPENDS = $(OBJS:.o=.d) $(TESTOBJ:.o=.d) $(MAINOBJ:.o=.d)


all: $(TARGET)
test: $(TESTTARGET) testrun

$(TARGET): $(MAINOBJ) $(OBJS)
	$(CC) -o $@ $(MAINOBJ) $(OBJS)

$(TESTTARGET): $(TESTOBJ) $(OBJS)
	$(CC) -o $@ $(TESTOBJ) $(OBJS) $(GTEST_LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@if [ ! -d $(OBJ_DIR) ]; \
		then echo "mkdir -p $(OBJ_DIR)"; mkdir -p $(OBJ_DIR); \
	fi
	$(CC) $(CFLAGS) -o $@ -c $<

$(TESTOBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	@if [ ! -d $(TESTOBJ_DIR) ]; \
		then echo "mkdir -p $(TESTOBJ_DIR)"; mkdir -p $(TESTOBJ_DIR); \
	fi
	$(CC) $(CFLAGS) -I $(SRC_DIR) -o $@ -c $<

testrun:
	$(TESTTARGET)


clean:
	$(RM) $(MAINOBJ) $(OBJS) $(TARGET) $(DEPENDS)

-include $(DEPENDS)

.PHONY: all clean
