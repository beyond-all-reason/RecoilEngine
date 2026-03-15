from conan import ConanFile, __version__ as conan_version
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import load
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Version
import os

required_conan_version = ">=2.0.0"

class RecoilConan(ConanFile):
    name = "recoil"
    license = "GPL-2.0-or-later"
    homepage = "https://github.com/beyond-all-reason/spring"
    description = "Recoil is an open source real time strategy game engine."
    topics = ("game-engine", "open-source")
    package_type = "application"
    version="105.0"

    exports_sources = "CMakeLists.txt", "src/*", "cmake/*", "cpack/*", "docs/*", "examples/*", "share/*", "tests/*",\
                      "VERSION", "README.md"
    no_copy_source = True
    settings = "os", "compiler", "build_type", "arch"
    options = {
    }
    default_options = {
    }

    @property
    def _testing_enabled(self):
        return not self.conf.get("tools.build:skip_test", default=True, check_type=bool)

    def requirements(self):
        self.requires("opengl/system")
        self.requires("glew/2.2.0")
        self.requires("devil/1.8.0")
        self.requires("zlib/1.3.1")
        if self.settings.os == "Linux":
            self.requires("libunwind/1.8.1", force=True)
            self.requires("expat/2.7.4")
        self.requires("zstd/1.5.6", override=True)
        self.requires("ogg/1.3.5")
        self.requires("vorbis/1.3.7")
        self.requires("sdl/2.32.10")
        self.requires("libcurl/7.88.1")
        self.requires("openssl/1.1.1v")
        self.requires("freetype/2.14.1")
        self.requires("fontconfig/2.17.1")
        if self.settings.os == "Windows":
            self.requires("minizip/1.3.1")
        self.requires("openal-soft/1.23.1")

    def configure(self):
        self.options["sdl"].shared = False
        self.options["sdl"].vulkan = False
        self.options["sdl"].opengles = False
        self.options["zlib"].shared = False
        self.options["libcurl"].shared = False
        if self.settings.os == "Linux":
            self.options["sdl"].wayland = False
            self.options["sdl"].pulse = False
            self.options["sdl"].alsa = False
            self.options["sdl"].hidapi = False
        if self.settings_build.os == "Windows":
            self.options["sdl"].directx = False

    def build_requirements(self):
        pass

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def validate(self):
        check_min_cppstd(self, "20")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        if self._testing_enabled:
            cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def layout(self):
        cmake_layout(self)
        build_folder = self.conf.get("user.cmake.cmake_layout:build_folder",
                                     default=f"cmake-build-{str(self.settings.build_type).lower()}")

        self.folders.build = build_folder
        self.folders.generators = f"{build_folder}/conan"
        self.folders.source = "."
        self.cpp.source.includedirs = "src"
