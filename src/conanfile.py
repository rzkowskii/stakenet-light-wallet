from conans import ConanFile, CMake
from conans.tools import Version
from conans.errors import ConanInvalidConfiguration

class StakenetConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [("boost/1.72.0"),
                ("zeromq/4.3.4"),
                ("openssl/1.1.1f"),
                ("zlib/1.2.11"),
                ("abseil/20200225.1@x9developers/stable"),
                ("berkleydb/4.8.30@x9developers/stable"),
                ("cares/1.15.0@x9developers/stable"),
                ("grpc/1.28.1@x9developers/stable"),
                ("leveldb/1.22@x9developers/stable"),
                ("protobuf/3.11.2@x9developers/stable"),
                ("secp256k1/1.0@x9developers/stable"),
                ("quazip/0.8.1@x9developers/stable"),
                ("lnd/0.12.0.8@x9developers/stable"),
                ("qt/5.15.0@x9developers/stable"),
                ("stakenet-updater/0.3.0@x9developers/stable"),
                ("rust-eth-lib/0.0.0.1@x9developers/stable")]

    default_options = {"boost:shared": False}
    generators = "cmake"

    def configure(self):
        self.options["openssl"].shared = self.settings.os == "Windows" or self.settings.os == "Linux"

        if self.settings.os == "Windows" and self.settings.compiler == "Visual Studio":
            compiler_version = Version(self.settings.compiler.version.value)
            if compiler_version < "15":
                raise ConanInvalidConfiguration("On Windows can only build with VS 2017 higher.")

    def requirements(self):
        if self.settings.os == "Linux":
            self.requires("breakpad-dump_syms/1.0@x9developers/stable")

        if self.settings.os == "Windows":
            self.requires("sentry-native/0.3.4@x9developers/stable")
            self.requires("msvc-runtime/14.0.0.1@x9developers/stable")
        else:
            self.requires("sentry-native/0.2.3@x9developers/stable")

    def imports(self):
        self.copy("crashpad_handler*", "bin", "bin")
        self.copy("protoc*", dst="bin", src="bin") # From bin to bin
        self.copy("grpc_cpp_plugin", dst="bin", src="bin")
        self.copy("dump_syms", dst="bin", src="bin")
        self.copy("lnd_*", dst="bin", src="bin")
        self.copy("updater*", dst="bin", src="bin")
        self.copy("checksum*", dst="bin", src="bin")

        self.copy("libcrypto*.dll", dst="bin", src="bin") # install dlls on windows
        self.copy("libssl*.dll", dst="bin", src="bin") # install dlls on windows
        self.copy("libcurl*.dll", dst="bin", src="bin") # install dlls on windows
        self.copy("vc*.dll", dst="bin/runtime", src="bin") # install dlls on windows
        self.copy("msvc*.dll", dst="bin/runtime", src="bin") # install dlls on windows

        self.copy("libssl*.so*", dst="lib", src="lib") # install dlls on linux
        self.copy("libcrypto*.so*", dst="lib", src="lib") # install dlls on linux
        self.copy("libcurl*.so*", dst="lib", src="lib") # install dlls on linux
        if self.settings.os == "Windows":
            self.copy("ffiweb3utils.lib", dst="lib", src="lib")
        else:
            self.copy("libffiweb3utils.a", dst="lib", src="lib")
