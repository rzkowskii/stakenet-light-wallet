#!/bin/bash

BUILD_HASH=$1
DEPLOY_ENV=$2
PLATFORM=$3

usage="$(basename "$0") BUILD_HASH DEPLOY_ENV PLATFORM -- program to deploy artifacts to aws s3 storage"

if [ -z "$BUILD_HASH" ]
then
    echo $usage
    echo "Missing BUILD_HASH value, can't continue"
    exit 1
fi

if [[ "$DEPLOY_ENV" != "production" && "$DEPLOY_ENV" != "staging" ]]; then
    echo $usage
    echo "DEPLOY_ENV has to be production or staging"
    exit 1
fi

if [[ "$PLATFORM" != "linux" && "$PLATFORM" != "win" && "$PLATFORM" != "osx" ]]; then
    echo $usage
    echo "PLATFORM has to be linux/win/osx"
fi


export PATH=$PWD/tools/bin:$PATH
export AWS_ACCESS_KEY_ID=AKIAVVJRXYD2N33SKN5W
export AWS_SECRET_ACCESS_KEY=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

BUCKET=s3://auto-updater-wallet-test/light-wallet/${DEPLOY_ENV}/${PLATFORM}

aws2 s3 cp --acl public-read ${BUILD_HASH}/${BUILD_HASH}.zip ${BUCKET}/builds/ || exit 1
aws2 s3 cp ${BUILD_HASH}/${BUILD_HASH}-symbols.zip ${BUCKET}/symbols/
aws2 s3 cp --acl public-read ${BUILD_HASH}/update.json ${BUCKET}/ || exit 1
