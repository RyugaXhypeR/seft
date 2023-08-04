D_MK = .target
D_SRC = src
D_INC = include

CF_SRC = $(wildcard $(D_SRC)/*.c)
HF_INC = $(wildcard $(D_INC)/*.h)
OF_SRC = $(CF_SRC:%.c=$(D_MK)/%.o)
OF_ALL = $(OF_SRC)
AF_SRC = $(CF_SRC:%.c=$(D_MK)/%.a)

D_INC_INSTALL = /usr/local/include
D_LIB_INSTALL = /usr/local/lib

OPT_LEVEL = 3

# If defined i.e D=DEBUG will display debug.
D = NDEBUG
C_FLAGS = -Wall -Wextra -g -O$(OPT_LEVEL) -D$(D) -fPIE -I$(D_SRC) -I$(D_INC) -lssh -Wno-stringop-truncation

.PHONY: all clean

-include $(DF_SRC)

all: $(AF_SRC)

# Compile the library into a static library.
$(AF_SRC): $(OF_SRC)
	@mkdir -p $(@D)
	ar rcs $@ $^
	ranlib $@

$(D_MK)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $(C_FLAGS) -o $@ $<

install: $(AF_SRC)
	mkdir -p $(D_INC_INSTALL) $(D_LIB_INSTALL)
	cp $(D_INC)/* $(D_INC_INSTALL)
	cp $^ $(D_LIB_INSTALL)

clean:
	$(RM) -r $(D_MK)
