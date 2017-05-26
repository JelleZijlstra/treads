OBJECTS=main.o util.o level.o maze.o gl_text.o audio.o
CXXFLAGS=-O0 -g -Wall -Werror -DMACOSX -Wno-deprecated-declarations -std=c++14 -I/opt/local/include -I/usr/local/include
LDFLAGS=-lphosg -framework OpenAL -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -g -std=c++14 -L/opt/local/lib -L/usr/local/lib -lglfw3
EXECUTABLES=treads

all: treads

treads: $(OBJECTS)
	g++ $(LDFLAGS) -o treads $^

clean:
	-rm -rf *.o $(EXECUTABLES)

.PHONY: clean
