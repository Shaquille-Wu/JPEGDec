CC = g++

all: jpeg_dec_test

jpeg_dec_test: main.cpp
	$(CC) -O0 -g main.cpp -o jpeg_dec_test -lturbojpeg -ljpeg

clean:
	rm jpeg_dec_test