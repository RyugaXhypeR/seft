D_MK = .target
D_SRC = src
D_INC = include

# For initial test runs
CF_TARGET = main.c
EF_TARGET = $(D_MK)/main

CF_SRC = $(wildcard $(D_SRC)/*.c)
HF_INC = $(wildcard $(D_INC)/*.h)
OF_SRC = $(CF_SRC:%.c=$(D_MK)/%.o)
OF_ALL = $(OF_SRC) $(D_MK)/main.o

D_INC_INSTALL = /usr/local/include
D_LIB_INSTALL = /usr/local/lib

OPT_LEVEL = 3

# If defined i.e D=DEBUG will display debug.
D = NDEBUG
C_FLAGS = -Wall -Wextra -g -O$(OPT_LEVEL) -D$(D) -fPIE -I$(D_SRC) -I$(D_INC) -lssh

.PHONY: all clean

-include $(DF_SRC)

all: $(EF_TARGET)

$(EF_TARGET): $(OF_ALL)
	$(CC) $(C_FLAGS) -o $@ $^

$(D_MK)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $(C_FLAGS) -o $@ $<

run: $(EF_TARGET)
	@$<

clean:
	$(RM) -r $(D_MK)
