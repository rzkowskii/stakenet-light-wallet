#!/bin/bash

APP_DIR=$1
SOURCES_DIR=$2
QMAKE=`which qmake`

function version_gt() { test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"; }

if test -z "${APP_DIR}" 
then
    echo "path to appdir is not valid"
    exit -1
fi

if test -z "${SOURCES_DIR}"
then
    echo "path to sources is not valid"
    exit -1
fi

QT_VERSION=$(${QMAKE} --version | tail -n1 | cut -d ' ' -f4)
MIN_QT_VERSION=5.12.1

if version_gt $MIN_QT_VERSION $QT_VERSION; then
     echo "$QT_VERSION is not supported minimum version: $MIN_QT_VERSION !"
     exit -1
fi

APP_EXE=${APP_DIR}/usr/bin/Release/Stakenet.exe

pushd .
cd ${SOURCES_DIR}
GIT_COMMIT=$(git rev-parse HEAD)
popd

mkdir ${GIT_COMMIT}
pushd .
cd ${GIT_COMMIT}

dump_syms ${APP_EXE} > Stakenet.sym #|| exit 1
strip ${APP_EXE}
REPORT_UID=$(head -n1 Stakenet.sym | cut -d' ' -f4)
REPORT_FOLDER_STRUCTURE=${PWD}/symbols/Stakenet/${REPORT_UID}
mkdir -p ${REPORT_FOLDER_STRUCTURE}
mv Stakenet.sym ${REPORT_FOLDER_STRUCTURE}/Stakenet.sym
CHECKSUM=$(${APP_DIR}/../../checksum/Release/checksum.exe ${APP_DIR}/usr/bin/Release)
pushd .
APP_VERSION=$(${APP_EXE} --version | tr -d '"' || exit 1)

jq-win64.exe -n \
        --argjson ver "$APP_VERSION" \
        --arg n "$GIT_COMMIT.zip" \
        --arg chs "$CHECKSUM" \
		'{version: $ver, remove:[], name:$n, checksum: $chs }' > update.json || exit 1

cd ${APP_DIR}/usr/bin/Release
zip -qr win.zip *
popd
mv ${APP_DIR}/usr/bin/Release/win.zip ${GIT_COMMIT}.zip
cd symbols; zip -qr ${GIT_COMMIT}-symbols.zip *; cd ../; mv symbols/${GIT_COMMIT}-symbols.zip ${GIT_COMMIT}-symbols.zip
rm -rf ./symbols
popd
