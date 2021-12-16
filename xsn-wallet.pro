TEMPLATE = subdirs

SUBDIRS += \
#    core_tests \
    core \
    app \
    bitcoin \
    networking \
    lightningswapsclient \
    lightningswapsdaemon \
#    src/lightwalletserviceclient \
    updater \
    libupdater \
    checksum \
    #bootstrap
    lndgrpcclient \
    lightningswaps \
    qgrpc \
    lndtools\
    common

!android {
SUBDIRS += \
    tor
}

app.subdir = src/app
core.subdir = src/core

#bootstrap.subdir = src/bootstrap
common.subdir = src/common
core_tests.subdir = src/core_tests
networking.subdir = src/networking
bitcoin.subdir = src/bitcoin
updater.subdir = src/updater
libupdater.subdir = src/libupdater
checksum.subdir = src/checksum
lndgrpcclient.subdir = src/lndgrpcclient
lightningswaps.subdir = src/lightningswaps
lightningswapsclient.subdir = src/lightningswapsclient
qgrpc.subdir = src/qgrpc
lndtools.subdir = src/lndtools
lightningswapsdaemon.subdir = src/lightningswapsdaemon

!android {
tor.subdir = src/tor
}

libupdater.depends = networking
core.depends = networking bitcoin lndtools lightningswaps
app.depends = core libupdater
lndgrpcclient.depends = core
updater.depends = libupdater
core_tests.depends = core
checksum.depends = libupdater
networking.depends = common
lightningswaps.depends = common
qgrpc.depends = common
lndtools.depends = common qgrpc
lightningswapsdaemon.depends = lightningswaps
lightningswaps.depends = bitcoin lndtools
#bootstrap.depends = core

!android {
tor.depends = bitcoin core
app.depends = tor
}


