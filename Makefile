CC		:= g++
FLAGS 	:= -g3 -Wall -Werror -Wextra -Wno-maybe-uninitialized -I ./include
LFLAGS  := -lpthread

EXEC_L  :=  ./bin

EXEC_C	:=  $(EXEC_L)/jobCommander
SRC_C 	:= 	./src/commander
BUILD_C	:= 	./build/commander
SRCS_C	:=	$(wildcard $(SRC_C)/*.cpp)
OBJS_C	:= 	$(patsubst $(SRC_C)/%.cpp, $(BUILD_C)/%.o, $(SRCS_C))

EXEC_S	:=  $(EXEC_L)/jobExecutorServer
SRC_S 	:= 	./src/server
BUILD_S	:= 	./build/server
SRCS_S	:=	$(wildcard $(SRC_S)/*.cpp)
OBJS_S	:= 	$(patsubst $(SRC_S)/%.cpp, $(BUILD_S)/%.o, $(SRCS_S))

.PHONY: all clean test

all: $(EXEC_C) $(EXEC_S)

# commander

$(EXEC_C): $(OBJS_C) | $(EXEC_L)
	$(CC) $(FLAGS) -o $@ $^

$(BUILD_C)/%.o: $(SRC_C)/%.cpp | $(BUILD_C)
	$(CC) $(FLAGS) -o $@ -c $^

# server

$(EXEC_S): $(OBJS_S) | $(EXEC_L)
	$(CC) $(FLAGS) -o $@ $^ $(LFLAGS)

$(BUILD_S)/%.o: $(SRC_S)/%.cpp | $(BUILD_S)
	$(CC) $(FLAGS) -o $@ -c $^


# make directories if they don't exist

$(EXEC_L):
	mkdir -p $@

$(BUILD_C):
	mkdir -p $@

$(BUILD_S):
	mkdir -p $@

clean:
	rm -rf $(EXEC_L)/* $(BUILD_C)/* $(BUILD_S)/*