#!/bin/bash

DEPLOY_TOOL=linuxdeployqt-continuous-x86_64.AppImage

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    --qmake)
    QMAKE="$2"
    shift # past argument
    shift # past value
    ;;
    --deploytool)
    DEPLOY_TOOL="$2"
    shift # past argument
    shift # past value
    ;;
    --projectpath)
    SOURCES_DIR="$2"
    shift # past argument
    shift # past value
    ;;
    --apppath)
    APP_EXE="$2"
    shift
    shift
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters


function version_gt() { test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"; }


QT_VERSION=$(${QMAKE} --version | tail -n1 | cut -d ' ' -f4)
MIN_QT_VERSION=5.12.1

if version_gt $MIN_QT_VERSION $QT_VERSION; then
     echo "$QT_VERSION is not supported minimum version: $MIN_QT_VERSION !"
     exit -1
fi

if ![ -x "$(command -v ${DEPLOY_TOOL})" ]; then
     echo 'Error: deployment tool is not installed.' >&2
     exit 1
fi

#pushd .
#cd ${SOURCES_DIR}
#GIT_COMMIT=$(git rev-parse HEAD)
#popd

#mkdir ${GIT_COMMIT}
#pushd .
#cd ${GIT_COMMIT}

EXCLUDE_LIST="libnspr4.so,libnss3.so,libnssutil3.so,libsmime3"

${DEPLOY_TOOL} ${APP_EXE} -qmake=$QMAKE -appimage -qmldir=${SOURCES_DIR}/src/app/qml -unsupported-allow-new-glibc -exclude-libs=${EXCLUDE_LIST}
#${DEPLOY_TOOL} ${APP_DIR}/usr/bin/updater -qmake=$QMAKE -appimage -unsupported-allow-new-glibc -exclude-libs=${EXCLUDE_LIST}
#${SOURCES_DIR}/modules/linux/breakpad/bin/dump_syms ${APP_EXE} > Stakenet.sym || exit 1
#strip ${APP_EXE}

#REPORT_UID=$(head -n1 Stakenet.sym | cut -d ' ' -f4)
#REPORT_FOLDER_STRUCTURE=${PWD}/symbols/Stakenet/${REPORT_UID}
#mkdir -p ${REPORT_FOLDER_STRUCTURE}
#mv Stakenet.sym ${REPORT_FOLDER_STRUCTURE}/Stakenet.sym
#CHECKSUM=$(${APP_DIR}/../../checksum/checksum ${APP_DIR}/usr)
#APP_VERSION=$(${APP_EXE} --version | tr -d '"' || exit 1)

#jq -n \
#    --argjson ver "$APP_VERSION" \
#    --arg n "$GIT_COMMIT.zip" \
#    --arg chs "$CHECKSUM" \
#	'{version: $ver, remove:[], name:$n, checksum: $chs }' > update.json || exit 1

#pushd .
#cd ${APP_DIR}/usr
#zip -qr linux.zip *
#popd
#mv ${APP_DIR}/usr/linux.zip ${GIT_COMMIT}.zip
#cd symbols; zip -qr ${GIT_COMMIT}-symbols.zip *; cd ../; mv symbols/${GIT_COMMIT}-symbols.zip ${GIT_COMMIT}-symbols.zip
#rm -rf ./symbols
#popd
