
CC=g++
AR=ar

AROPTS=rcs

LIBS =
OBJS = darx.o
OUTPUT=darx

all: static

static: $(OBJS)
	$(AR) $(AROPTS) lib$(OUTPUT).a $(OBJS)

%.o: %.cpp
	$(CC) $< -c -o $@

clean:
	rm $(OBJS) lib$(OUTPUT).a

run: all
	./$(OUTPUT)