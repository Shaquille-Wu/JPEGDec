PROJECT_NAME=FFmpeg
BUILD_TYPE=Release
BUILD_CMD=build
#TARGET_OS support linux and macOSX
TARGET_OS=linux
#TARGET_ARCH support x86_64
TARGET_ARCH=x86_64

while getopts ":c:t:a:o:q" opt
do
    case $opt in
        c)
        BUILD_CMD=$OPTARG
        ;;    
        t)
        BUILD_TYPE=$OPTARG
        ;;
        a)
        TARGET_ARCH=$OPTARG
        ;;
        o)
        TARGET_OS=$OPTARG
        ;;        
        ?)
        echo "unknow parameter: $opt"
        exit 1;;
    esac
done

CUR_TOP_PATH=${PWD}
BUILD_PLATFORM=${TARGET_ARCH}-${TARGET_OS}

mkdir -p ./build/${BUILD_TYPE}/${BUILD_PLATFORM}/yasm
cd ./build/${BUILD_TYPE}/${BUILD_PLATFORM}/yasm
echo entring ${PWD} for ${BUILD_CMD} yasm start
mkdir -p install
YASM_INSTALL_DIR=${PWD}/install
${CUR_TOP_PATH}/yasm-1.3.0/configure --prefix=${YASM_INSTALL_DIR}
make -j 8
make install
echo entring ${PWD} for ${BUILD_CMD} yasm stop

cd ${CUR_TOP_PATH}

export PATH=${YASM_INSTALL_DIR}/bin:${PATH}
export CPATH="$YASM_INSTALL_DIR/include:$CPATH"
export LD_LIBRARY_PATH="$YASM_INSTALL_DIR/lib:$LD_LIBRARY_PATH"
export LIBRARY_PATH="$YASM_INSTALL_DIR/lib:$LIBRARY_PATH"

mkdir -p ./build/${BUILD_TYPE}/${BUILD_PLATFORM}/${PROJECT_NAME}
cd ./build/${BUILD_TYPE}/${BUILD_PLATFORM}/${PROJECT_NAME}
echo entring ${PWD} for ${BUILD_CMD} ${PROJECT_NAME} start
echo "ffmpeg compile start"
mkdir -p install
FFMPEG_INSTALL_DIR=${PWD}/install
CFG_CMD_LINE="--enable-gpl"
CFG_CMD_LINE="${CFG_CMD_LINE} --enable-nonfree"
CFG_CMD_LINE="${CFG_CMD_LINE} --enable-filter=delogo"
CFG_CMD_LINE="${CFG_CMD_LINE} --disable-optimizations"
CFG_CMD_LINE="${CFG_CMD_LINE} --enable-shared"
CFG_CMD_LINE="${CFG_CMD_LINE} --enable-pthreads"
if [ "$TARGET_OS" == "macOSX" ] ; then
    CFG_CMD_LINE="${CFG_CMD_LINE} --enable-videotoolbox"
    CFG_CMD_LINE="${CFG_CMD_LINE} --enable-version3"
    CFG_CMD_LINE="${CFG_CMD_LINE} --enable-hardcoded-tables"
    CFG_CMD_LINE="${CFG_CMD_LINE} --cc=clang"
    #CFG_CMD_LINE="${CFG_CMD_LINE} --host-cflags="
    #CFG_CMD_LINE="${CFG_CMD_LINE} --host-ldflags="
elif [ "$TARGET_OS" == "linux" ] ; then
    echo "no extra cmd for linux"
else
    echo "unknown platform"
    exit 0
fi

RELEASE_OPT="-O3"

if [ "$BUILD_TYPE" == "Debug" ] ; then
    CFG_CMD_LINE="${CFG_CMD_LINE} --enable-debug"
else
    CFG_CMD_LINE="${CFG_CMD_LINE} --extra-cflags=${RELEASE_OPT}"
fi

if [ "$BUILD_CMD" == "build" ]; then
    ${CUR_TOP_PATH}/FFmpeg/configure --prefix=${FFMPEG_INSTALL_DIR} ${CFG_CMD_LINE}
    make clean
    make -j8
    make install
elif 
    [ "$BUILD_CMD" == "clean" ]; then
    make clean
else
    echo "unknown cmd"
    exit 0
fi

cd ${CUR_TOP_PATH}
echo leaving ${PWD} for ${BUILD_CMD} ${PROJECT_NAME} end