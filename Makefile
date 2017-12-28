OBJECTS=main.o level.o maze.o gl_text.o audio.o
CXXFLAGS=-O0 -g -Wall -Werror -DMACOSX -Wno-deprecated-declarations -std=c++14 -I/opt/local/include -I/usr/local/include
LDFLAGS=-lphosg -framework OpenAL -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -g -std=c++14 -L/opt/local/lib -L/usr/local/lib -lglfw3
EXECUTABLES=treads

all: treads.app/Contents/MacOS/treads

treads: $(OBJECTS)
	g++ $(LDFLAGS) -o treads $^

treads.app/Contents/MacOS/treads: treads treads.icns media/levels.json
	./make_bundle.sh treads treads com.fuzziqersoftware.treads treads
	cp media/* treads.app/Contents/Resources/

clean:
	-rm -rf *.o $(EXECUTABLES) treads.app

.PHONY: clean
