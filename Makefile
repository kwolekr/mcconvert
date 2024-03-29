PROGNAME = mcconvert
rm = /bin/rm -f
CC = cc
CXX = c++
AS = as
DEFS = -Wno-multichar
INCLUDES = -I. -I/usr/local/include
LIBS = -L/usr/local/lib -lz -lsqlite3
DEFINES = $(INCLUDES) $(DEFS) -DSYS_UNIX=1

CFLAGS = -pipe -Wall -O3 $(DEFINES)
CXXFLAGS = -pipe -Wall -O3 $(DEFINES)
ASFLAGS = 

SOURCES = db.c \
mapcontent.c \
mcconvert.c \
vector.c

OBJECTS = ${SOURCES:.c=.o}
SRCS = ${addprefix src/,$(SOURCES)}
OBJS = ${addprefix obj/,$(OBJECTS)}

.SILENT:

obj/%.o: src/%.c
	mkdir -p $(@D);
	#$(rm) $@;
	if ${CC} ${CFLAGS} -c -o $@ $<; then \
		printf "\033[32mbuilt $@.\033[m\n"; \
	else \
		printf "\033[31mbuild of $@ failed!\033[m\n"; \
		false; \
	fi
	
obj/%.o: src/%.cpp
	mkdir -p $(@D);
	$(rm) $@;
	if ${CXX} ${CXXFLAGS} -c -o $@ $<; then \
		printf "\033[32mbuilt $@.\033[m\n"; \
	else \
		printf "\033[31mbuild of $@ failed!\033[m\n"; \
		false; \
	fi
	
obj/%.o: src/%.s
	mkdir -p $(@D);
	$(rm) $@;
	if ${AS} ${ASFLAGS} $< -o $@; then \
		printf "\033[32mbuilt $@.\033[m\n"; \
	else \
		printf "\033[31mbuild of $@ failed!\033[m\n"; \
		false; \
	fi
	
all: $(PROGNAME)

debug: CFLAGS = -pipe -Wall -g $(DEFINES)
debug: $(PROGNAME)

$(PROGNAME) : $(OBJS)
	if $(CC) $(CFLAGS) -o $(PROGNAME) $(OBJS) $(LIBS); then \
		printf "\033[32mlinked $@.\033[m\n"; \
	else \
		printf "\033[31mlink of $@ failed!\033[m\n"; \
		false; \
	fi

clean:
	$(rm) $(PROGNAME) core *~
	$(rm) -rf obj/*
