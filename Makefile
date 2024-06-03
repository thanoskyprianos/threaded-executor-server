CC		:= g++
FLAGS 	:= -g3 -Wall -Werror -Wextra -Wno-maybe-uninitialized -I ./include
LFLAGS  := -lpthread

EXEC_L  :=  ./bin
OUTPUT	:=  ./output

EXEC_C	:=  $(EXEC_L)/jobCommander
SRC_C 	:= 	./src/commander
BUILD_C	:= 	./build/commander
SRCS_C	:=	$(wildcard $(SRC_C)/*.cpp)
OBJS_C	:= 	$(patsubst $(SRC_C)/%.cpp, $(BUILD_C)/%.o, $(SRCS_C))
INCL_C  := 	-I ./include/commander

EXEC_S	:=  $(EXEC_L)/jobExecutorServer
SRC_S 	:= 	./src/server
BUILD_S	:= 	./build/server
SRCS_S	:=	$(wildcard $(SRC_S)/*.cpp)
OBJS_S	:= 	$(patsubst $(SRC_S)/%.cpp, $(BUILD_S)/%.o, $(SRCS_S))
INCL_S	:=	-I ./include/server

.PHONY: all clean test

all: $(EXEC_C) $(EXEC_S) | $(OUTPUT)

# commander

$(EXEC_C): $(OBJS_C) | $(EXEC_L)
	$(CC) $(FLAGS) $(INCL_C) -o $@ $^

$(BUILD_C)/%.o: $(SRC_C)/%.cpp | $(BUILD_C)
	$(CC) $(FLAGS) $(INCL_C) -o $@ -c $^

# server

$(EXEC_S): $(OBJS_S) | $(EXEC_L)
	$(CC) $(FLAGS) $(INCL_S) -o $@ $^ $(LFLAGS)

$(BUILD_S)/%.o: $(SRC_S)/%.cpp | $(BUILD_S)
	$(CC) $(FLAGS) $(INCL_S) -o $@ -c $^


# make directories if they don't exist

$(EXEC_L):
	mkdir -p $@

$(BUILD_C):
	mkdir -p $@

$(BUILD_S):
	mkdir -p $@

$(OUTPUT):
	mkdir -p $@

clean:
	rm -rf $(EXEC_L)/* $(BUILD_C)/* $(BUILD_S)/* $(OUTPUT)/*