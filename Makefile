D_MK = .target
D_SRC = src
D_INC = include

# For initial test runs
CF_TARGET = main.c

CF_SRC = $(wildcard $(D_SRC)/*.c)
HF_INC = $(wildcard $(D_INC)/*.h)
OF_SRC = $(wildcard $(D_MK)/*.o)

D_INC_INSTALL = /usr/local/include
D_LIB_INSTALL = /usr/local/lib

OPT_LEVEL = 3

# If defined i.e D=DEBUG will display debug.
D = NDEBUG
C_FLAGS = -Wall -Wextra -g -O$(OPT_LEVEL) -D$(D) -I$(D_SRC) -I$(D_INC)


.PHONY: all clean

all: $(D_MK) $(D_MK)/$(CF_TARGET)

$(D_MK):
	mkdir -p $(D_MK)

$(D_MK)/$(CF_TARGET): $(CF_SRC) $(HF_INC)
	$(CC) $(C_FLAGS) -o $@ $^

clean:
	$(RM) -r $(D_MK)
