# Project structure

- `depends` - utility scritps for building dependencies
- `cmake` - scripts that are used to build/deploy project
- `deploy` - utility scripts for reducing deployment complexity
- `src` - all source files
   - `app` - main UI application, contains QML files
   - `checksum` - utility that is used to verify update checksums
   - `common` - common files shared between other modules
   - `core` - core logic of light wallet, contains models, viewmodels, data models and majority of business logic
   - `core_tests` - tests for core logic
   - `crashreporting` - tools for crashreporting
   - `ethcore` - functionality specifically for ETH chain
   - `libupdater` - logic for applying updates
   - `lightningswaps` - core logic for L2 swaps
   - `lightningswapsclient` - utility lssd client for testing
   - `lightningswapsdaemon` - main LSSD daemon
   - `lightwalletserviceclient` - utility lightwallet grpc client for testing
   - `lndgrpcclient` - utility grpc client for testing
   - `lndtools` - tools for interacting with lnd
   - `networking` - networking, API access tools
   - `qgrpc` - utility for google GRPC framework
   - `tor` - logic for using tor network
   - `tradingbot` - implementation of grid trading bot
   - `walletcore` - core logic that implements private key management
   
# Building

This project supports [CMake](https://cmake.org/) out of the box. CMake is now used as main build tool.

## Requirments

- CMake >= 3.16.3
- Qt 5.15.0
- Compiler with C++17 support
- Qt WebEngine

## Install latest cmake

To build the project you will need minimum CMake >= 3.16.3

```
cmake --version
```

Check cmake version, if it's lower than 3.16.3, update to latest cmake.
For ubuntu it can be done with next commands:

```
wget https://github.com/Kitware/CMake/releases/download/v3.16.3/cmake-3.16.3-Linux-x86_64.sh
chmod +x cmake-3.16.3-Linux-x86_64.sh
sudo ./cmake-3.16.3-Linux-x86_64.sh --skip-license --prefix=/usr
```

## Build with conan

Setup conan remote(once)

```
conan remote add <REMOTE_NAME> https://stakenetwallet.jfrog.io/artifactory/api/conan/light-wallet-conan
```

```
mkdir build
cd build
conan install path_to_sources/src -s compiler.libcxx=libstdc++11
cmake -DCMAKE_BUILD_TYPE=Release path_to_sources/src
cmake --build . --parallel 4
```

# Release process

This section describes everything related to release process. How to properly setup versions, where to push released. How to make it right.

## Environment

We support two environments `staging` and `production`, those environments are separated using separate branches.

`master` branch points to `production` env.

`staging` branch points to `staging` env.

## Versioning

Our software uses versinong in standard `cmake` format: `<major>.<minor>.<patch>.<tweak>`

Production released have to have at least different `<patch>` version, next scenario is not ok:

1. Current production version 0.1.1.4
2. Developer wants to push 0.1.1.5 (DON'T DO THIS)

Instead do this:

1. Current production version 0.1.1.4
2. Developer wants to push 0.1.2.0 (CORRECT WAY)

This way we will have an ability to iterate over staging fixes without skipping versions on production.

## Workflow

All development is done on feature/bugfix branches, once it's ready to merge they are merged into `develop` branch. 

Once we are release ready we merge `develop` into `staging` for testing. At this point CI build agents prepare staging builds and release them to to `staging` environment. This step can be repeated a few times if we found bugs during release process. 

Once we are bug free and ready release to general public we merge `staging` into `master`. At this point CI build agents prepare production builds and release them to general public on `production` environment. 

## Steps before releasing

1. Fill changelog with latest version.
2. Change version in `src/CMakeLists.txt`
3. Change version in `src/lightningswapsdaemon/CMakeLists.txt`
4. Commit

## Sample workflow

Sample scenario:
```
Current production version: 0.1.3.1
Current staging version: 0.1.3.1
```

1. Alice works on `feature/important-feature` branch, it's ready to be merged.
2. Alice creates PR to `develop`, Bob reviews PR and it's ready to be merged.
3. Alice merges PR to `develop`. 
4. Alice performs [steps](#steps-before-releasing) and pushes a new commit to `develop`. 
5. Alice is ready to push her feature to testers. Alice merges `develop` into `staging`. Current staging version: `0.1.4.0`
6. Carol starts testing staging build with version `0.1.4.0`. Carol founds a few issues and reports to Alice. 
7. Alice fixes issue and makes a new staging release with version `0.1.4.1`. 
8. Carol confirms that issues are fixed and build is ready for production. 
9. Alice pulls `staging` into `master` branch, triggering CI build to master and release to public.

After those steps our versions will be:
```
Current production version: 0.1.4.1
Current staging version: 0.1.4.1
```

# Deploying

## Linux

To deploy application on linux, simply call 
```
cmake --install . --component Stakenet
or 
cmake --install . --component lssd
```

this will prepare "package" folder in build folder, where zip will be located or any other extra files.

### Deploy update to S3

To deploy update to s3 call: 
```
cmake -DDEPLOY_ENV="staging" -DPLATFORM="win" -DUPDATE_DIR=build-dir/package/e31b489e56b846f1d2ad44787d8536bb6499367e -P cmake/s3-deploy-update.cmake
```
You can configure env, platform and update dir

# Maintaining of CI agents

Previously we were using bintray artifactory to manage dependencies. Bintray was deprecated, temporary we are using jfrog. To upload artifacts from local cache to jfrog execute next commands:
```
conan remote add jfrog-x9developers https://stakenetwallet.jfrog.io/artifactory/api/conan/light-wallet-conan
conan user -p <PASSWORD> -r jfrog-x9developers <USERNAME>
conan upload *@x9developers/stable -r=jfrog-x9developers --all
```
