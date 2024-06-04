EXEC_L  :=  ./bin
SRC_C 	:= 	./src/commander
BUILD_C	:= 	./build/commander
SRC_S 	:= 	./src/server
BUILD_S	:= 	./build/server
INCL_S	:=	-I ./include/server

CC		:= g++
FLAGS 	:= -g3 -Wall -Werror -Wextra -I ./include
LFLAGS  := -lpthread

EXEC_C	:=  $(EXEC_L)/jobCommander
SRCS_C	:=	$(wildcard $(SRC_C)/*.cpp)
OBJS_C	:= 	$(patsubst $(SRC_C)/%.cpp, $(BUILD_C)/%.o, $(SRCS_C))
INCL_C  := 	-I ./include/commander

EXEC_S	:=  $(EXEC_L)/jobExecutorServer
SRCS_S	:=	$(wildcard $(SRC_S)/*.cpp)
OBJS_S	:= 	$(patsubst $(SRC_S)/%.cpp, $(BUILD_S)/%.o, $(SRCS_S))

.PHONY: all clean help

all: $(EXEC_C) $(EXEC_S)
	@echo
	@echo 'Finished compiling. Run `make help` for more info'

help:
	@cat help

# commander

$(EXEC_C): $(OBJS_C) | $(EXEC_L)
	@$(CC) $(FLAGS) $(INCL_C) -o $@ $^
	@echo "Compiled $@"

$(BUILD_C)/%.o: $(SRC_C)/%.cpp | $(BUILD_C)
	@$(CC) $(FLAGS) $(INCL_C) -o $@ -c $^
	@echo "Compiled $@"

# server

$(EXEC_S): $(OBJS_S) | $(EXEC_L)
	@$(CC) $(FLAGS) $(INCL_S) -o $@ $^ $(LFLAGS)
	@echo "Compiled $@"

$(BUILD_S)/%.o: $(SRC_S)/%.cpp | $(BUILD_S)
	@$(CC) $(FLAGS) $(INCL_S) -o $@ -c $^
	@echo "Compiled $@"


# make directories if they don't exist

$(EXEC_L):
	mkdir -p $@

$(BUILD_C):
	mkdir -p $@

$(BUILD_S):
	mkdir -p $@

clean:
	rm -rf $(EXEC_L)/* $(BUILD_C)/* $(BUILD_S)/*