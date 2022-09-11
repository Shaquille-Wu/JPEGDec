PROJECT_NAME=libjpeg-turbo
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

YASM_INSTALL_DIR=${CUR_TOP_PATH}/build/Release/${BUILD_PLATFORM}/yasm/install

export PATH=${YASM_INSTALL_DIR}/bin:${PATH}
export CPATH="$YASM_INSTALL_DIR/include:$CPATH"
export LD_LIBRARY_PATH="$YASM_INSTALL_DIR/lib:$LD_LIBRARY_PATH"
export LIBRARY_PATH="$YASM_INSTALL_DIR/lib:$LIBRARY_PATH"

mkdir -p ./build/${BUILD_TYPE}/${BUILD_PLATFORM}/${PROJECT_NAME}
cd ./build/${BUILD_TYPE}/${BUILD_PLATFORM}/${PROJECT_NAME}
echo entring ${PWD} for ${BUILD_CMD} ${PROJECT_NAME} start
echo "libjpeg-turbo compile start"
mkdir -p install
LIBJPEG_TURBO_INSTALL_DIR=${PWD}/install
CMAKE_CMD_LINE="-GNinja "
CMAKE_CMD_LINE="${CMAKE_CMD_LINE} -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR} "
CMAKE_CMD_LINE="${CMAKE_CMD_LINE} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} "


if [ "$BUILD_CMD" == "build" ]; then
    cmake ${CUR_TOP_PATH}/libjpeg-turbo ${CMAKE_CMD_LINE}
    ninja clean
    ninja -j8
    ninja install
elif 
    [ "$BUILD_CMD" == "clean" ]; then
    ninja clean
else
    echo "unknown cmd"
    exit 0
fi
echo "libjpeg-turbo compile stop"
cd ${CUR_TOP_PATH}
echo leaving ${PWD} for ${BUILD_CMD} ${PROJECT_NAME} end