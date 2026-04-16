CC=gcc
CFLAGS=-O3 -Wall -Iinclude
LDFLAGS=-Lfreeglut/lib/x64
LIBS=-lfreeglut -lglu32 -lopengl32 -lm

all: neuron3d

# Questa regola dice al compilatore di cercare i file dentro la cartella src
neuron3d: src/main.c src/neuron.c src/render.c src/stdp.c
	$(CC) $(CFLAGS) src/main.c src/neuron.c src/render.c src/stdp.c -o neuron3d.exe $(LDFLAGS) $(LIBS)

clean:
	del /f neuron3d.exe

run: neuron3d
	./neuron3d.exe
