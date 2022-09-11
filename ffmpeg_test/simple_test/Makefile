CC = g++

#FFMPEG_INSTALL_DIR=/Users/xiaow/WorkSpace/JPEGCodec/build/Release/x86_64-macOSX/ffmpeg/install
#FFMPEG_INC_DIR=${FFMPEG_INSTALL_DIR}/include
#FFMPEG_LIB_DIR=${FFMPEG_INSTALL_DIR}/lib

all: jpeg_dec_test

jpeg_dec_test: main.cpp
	$(CC) -O0 -g -std=c++14 main.cpp KyMEM.cpp -o jpeg_dec_test -lavcodec -lavfilter -lavformat -lavutil -lswresample -lswscale

clean:
	rm jpeg_dec_test