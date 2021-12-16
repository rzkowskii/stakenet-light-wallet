#!/bin/bash


mkdir -p tools/bin
cd tools
# wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage -P bin || exit 1
# wget -O bin/jq https://github.com/stedolan/jq/releases/download/jq-1.6/jq-linux64 || exit 1
curl "https://d1vvhvl2y92vvt.cloudfront.net/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip" || exit 1 
curl -L "https://github.com/getsentry/sentry-cli/releases/download/1.52.1/sentry-cli-Linux-x86_64" -o "$PWD/bin/sentry-cli" || exit 1
unzip awscliv2.zip
rm awscliv2.zip
chmod +x $PWD/bin/*
./aws/install --install-dir $PWD/aws-cli --bin-dir $PWD/bin || exit 1
rm -r aws
chmod +x $PWD/bin/*

