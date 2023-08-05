D_MK = .target
D_SRC = src
D_INC = include

CF_SRC = $(wildcard $(D_SRC)/*.c)
HF_INC = $(wildcard $(D_INC)/*.h)
OF_SRC = $(CF_SRC:%.c=$(D_MK)/%.o)
DF_SRC = $(OF_SRC:%.o=%.d)
OF_ALL = $(OF_SRC)
AF_SRC = $(CF_SRC:%.c=$(D_MK)/%.a)
CF_SRC_NO_DIR = $(notdir $(CF_SRC))
HF_INC_NO_DIR = $(notdir $(HF_INC))
HF_INST = $(HF_INC_NO_DIR:%.h=$(D_INC_INSTALL)/%.h)
AF_INST = $(CF_SRC_NO_DIR:%.c=$(D_LIB_INSTALL)/%.a)

D_INC_INSTALL = /usr/local/include
D_LIB_INSTALL = /usr/local/lib

# If defined i.e D=DEBUG will display debug.
D = NDEBUG
LINK_FLAGS = -lssh
INC_FLAGS = -I$(D_SRC) -I$(D_INC)
OPT_FLAG = -O3
IGNORE_FLAGS = -Wno-stringop-truncation
LINTER_FLAGS = -Wall -Wextra -Wpedantic
C_FLAGS = $(LINTER_FLAGS) $(IGNORE_FLAGS) -g $(OPT_FLAG) $(INC_FLAGS) $(LINK_FLAGS) -D=$(D)

.PHONY: all clean

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

-include $(DF_SRC)

run:
	$(CC) -L./.target/src/commands.a ./.target/src/sftp_client.a -Isrc -Iinclude main.c -o $(D_MK)/main && $(D_MK)/main
uninstall:
	$(RM) $(HF_INST) $(AF_INST)

clean:
	$(RM) -r $(D_MK)
