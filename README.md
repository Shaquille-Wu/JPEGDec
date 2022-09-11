# JPEGDec

This repository is a demo to compare the performance between ffmpeg and turbo-jpeg
## download and compile ffmpeg and turbo-jpeg

**1) Download yasm**  

```
wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
```

and then, you should decompress it as follwoing:  
```
tar -xzvf yasm-1.3.0.tar.gz
```

**2) Download ffmpeg**  

```
git clone https://github.com/FFmpeg/FFmpeg.git
```

**3) Download the turbo-jpeg**  

```
git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git
```

**4) Compile ffmpeg**

We provide a script to finish the compile: build_ffmpeg.sh, and you should build ffmpeg as follwoing:  

```
chmod +x ./build_ffmpeg.sh
./build_ffmpeg.sh
```

**5) Compile turbo-jpeg**

We provide a script to finish the compile: build_tj.sh, and you should build turbo-jpeg as follwoing:  

```
chmod +x ./build_tj.sh
./build_tj.sh
```

this script should be executed after "build_ffmpeg.sh", because this project depends on yasm, and yasm is built in "build_ffmpeg.sh"  

**6) setup required enviornment**

Our demos are depend on ffmpeg and turbo-jpeg, so, you should setup enviornment before build our demos  
We provide a script to finish this work: jpeg_env.sh, command as following:  

```
source ./jpeg_env.sh
```

you should modify the path in "jpeg_env.sh", if the paths of ffmpeg and turbo-jpeg are different from it  

## ffmpeg_test

there are 4 demos in this folder, and they depend on ffmpeg     

* simple_test: just decode jpeg file, and save the decoded image as *.bmp file  
* simple_test_time: decode jpeg file, run 1000 times, and record the time cost  
* loop_image: decode jpeg file in an folder(imagenet1K), record the time cost, you should modify the folder path before compile it  
* loop_image_multithread: decode jpeg file in an folder(imagenet1K), we implement it through omp(multithread), record the time cost, you should modify the folder path before compile it  

above demos, you should execute "make" before run them, and all of them are named as "jpeg_dec_test"  

## turbojpeg_test

there are 3 demos in this folder, and they depend on turbo-jpeg  

* simple_test: just decode jpeg file, and save the decoded image as *.bmp file  
* simple_test_time: decode jpeg file, run 1000 times, and record the time cost  
* loop_image_multithread: decode jpeg file in an folder(imagenet1K), we implement it through omp(multithread), record the time cost, you should modify the folder path before compile it  

above demos, you should execute "make" before run them, and all of them are named as "jpeg_dec_test"  

