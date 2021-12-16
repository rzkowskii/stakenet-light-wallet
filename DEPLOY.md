Deploy for mac, sample: `~/Qt/5.12.0/clang_64/bin/macdeployqt /Users/yuraolex/Work/Reps/build-xsn-wallet-Desktop_Qt_5_12_0_clang_64bit3-Release/src/app/appdir/usr/bin/XSNWallet.app -qmldir=/Users/yuraolex/Work/Reps/xsn-wallet/src/app/qml`
Deploy for linux, sample: `~/Downloads/linuxdeployqt-continuous-x86_64.AppImage /home/durkmurder/Work/Reps/build-xsn-wallet-Desktop_Qt_5_12_1_GCC_64bit-Release/src/app/appdir/usr/bin/XSNWallet -qmake=/home/durkmurder/Qt/5.12.1/gcc_64/bin/qmake -appimage -qmldir=/home/durkmurder/Work/Reps/xsn-wallet/src/app/qml`
Deploy for win, sample: `/e/Qt/5.12.1/msvc2015_64/bin/windeployqt.exe /e/Work/Reps/build-xsn-wallet-Desktop_Qt_5_12_1_MSVC2015_64bit-Release/src/app/appdir/usr/bin/XSNWallet.exe -qmldir /e/Work/Reps/xsn-wallet --dir /e/Work/Reps/build-xsn-wallet-Desktop_Qt_5_12_1_MSVC2015_64bit-Release/deploy`

How to make a proper build & deploy update:

0. Make sure that everything is committed and you have chosen APPLICATION_VERSION and STAGING_ENV in ApplicationViewModel.cpp
1. Prepare a release build to build with breakpad support make sure that you have added `CONFIG+=force_debug_info` in project settings for whole project, 
since release builds don't support debug symbols out of the box.
2. Go to deploy folder and execute one of deployment scripts which will prepare a folder structure with release files. 
3. Make sure that application is stable and doesn't depend on local modules, this can happen only if deployment tool failed to copy some dependencies.
In this case you will need to resolve those manually and run deploy script again. Usually it's a one time process.
4. Upload artifacts: 
	- zip with build to this url: `https://console.aws.amazon.com/s3/buckets/auto-updater-wallet-test/light-wallet/{staging or production}/{win or linux or osx}/builds`
	- `update.json` to this url: `https://console.aws.amazon.com/s3/buckets/auto-updater-wallet-test/light-wallet/{staging or production}/{win or linux or osx}`
	- symbols to this url: `https://console.aws.amazon.com/s3/buckets/auto-updater-wallet-test/light-wallet/{staging or production}/{win or linux or osx}/symbols`

5. Make sure that all of those artifacts are accessible for public download, this can be made by selecting in combobox when uploading 


Scripts in deploy folder require some preconditions: 
1. You need to have `jq` installed and it has to be in your path. `https://stedolan.github.io/jq/`
2. You need to have `qmake` in path with desired qt version, minimum 5.12.1
3. For linux deployment you need to have linuxdeployqt tool, which can be found here: `https://github.com/probonopd/linuxdeployqt/releases`


======================================

LSSD Deployment

To deploy lssd you can use next script from deploy folder:

0. ./lssd-deploy-ubuntu.sh ~/Work/Reps/build-xsn-wallet-Desktop_Qt_5_12_2_GCC_64bit-Debug/src/lightningswapsdaemon/appdir

Just run it and pass path to build folder where lssd was built. It will make a self contained package in appdir.