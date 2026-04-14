from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout

required_conan_version = ">=2.0.0"

class RecoilConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    options = {}
    default_options = {}

    def requirements(self):
        self.requires("opengl/system")
        self.requires("devil/1.8.0")
        self.requires("zlib/1.3.1")
        if self.settings.os == "Linux":
            self.requires("libunwind/1.8.1", force=True)
        self.requires("zstd/1.5.7", override=True)
        self.requires("ogg/1.3.5")
        self.requires("vorbis/1.3.7")
        self.requires("sdl/2.32.10")
        self.requires("libcurl/8.19.0")
        # There newer 3.5.* and 3.6.* branches fail to compile for us on mingw version
        # we use at the moment due to https://github.com/openssl/openssl/issues/29818
        # but the 3.4.* branch is still maintained so we can use it.
        self.requires("openssl/3.4.4")
        self.requires("freetype/2.14.1")
        self.requires("fontconfig/2.17.1")
        if self.settings.os == "Windows":
            self.requires("minizip/1.3.1")
        # This version of openal-soft can fail on newer libraries versions, due to change
        # in header files https://github.com/conan-io/conan-center-index/issues/27648, but
        # no newer version in conan registry yet.
        self.requires("openal-soft/1.23.1")

    def configure(self):
        self.options["sdl"].shared = False
        self.options["sdl"].vulkan = False
        self.options["sdl"].opengles = False
        self.options["zlib"].shared = False
        self.options["libcurl"].shared = False
        self.options["libcurl"].with_ssl = "openssl"  # Want openssl on all platforms
        self.options["libcurl"].with_zstd = True
        # We want HTTP2 for pr-downloader but unfortunatelly it's at the moment broken
        # on windows https://github.com/conan-io/conan-center-index/issues/29909
        # self.options["libcurl"].with_nghttp2 = True
        self.options["libcurl"].with_ftp = False
        self.options["libcurl"].with_dict = False
        self.options["libcurl"].with_file = False
        self.options["libcurl"].with_gopher = False
        self.options["libcurl"].with_imap = False
        self.options["libcurl"].with_mqtt = False
        self.options["libcurl"].with_pop3 = False
        self.options["libcurl"].with_rtsp = False
        self.options["libcurl"].with_smb = False
        self.options["libcurl"].with_smtp = False
        self.options["libcurl"].with_telnet = False
        self.options["libcurl"].with_form_api = False
        self.options["libcurl"].with_websockets = False

        if self.settings.os == "Linux":
            self.options["sdl"].wayland = True
            self.options["sdl"].pulse = False
            self.options["sdl"].alsa = False
            self.options["sdl"].hidapi = False
        if self.settings_build.os == "Windows":
            self.options["sdl"].directx = False

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def layout(self):
        cmake_layout(self)
        build_folder = self.conf.get("user.cmake.cmake_layout:build_folder",
                                     default=f"build-{str(self.settings.build_type).lower()}")
        self.folders.build = build_folder
        self.folders.generators = f"{build_folder}/conan"
