from conan import ConanFile
# from conan.tools.scm import Git
from conan.tools.files import copy
from conan.tools.layout import basic_layout
# from conan.tools.cmake import cmake_layout, CMakeDeps, CMake, CMakeToolchain
import os

class Allocators(ConanFile):
    name = "allocators"
    description = ""
    author = "dd99"
    # license = ""
    # homepage = "http://"
    # url = ""
    topics = ("memory", "low-level", "header-only")

    package_type = "header-library"
    # settings = "os", "arch", "compiler", "build_type"
    settings = None
    no_copy_source = True
    exports_sources = "*CMakeLists.txt", "allocators/*", "LICENCE*"


    def layout(self):
        basic_layout(self)
        # cmake_layout(self)

    def package_id(self):
        self.info.clear()

    def source(self):
        # git_url = "/home/dd99/Documents/programming/..."
        # git = Git(self)
        # git.clone(url=git_url, target=".")
        # tag_str = name + self.version
        # git.checkout(commit=tag_str)
        pass

    # def generate(self):
    #     deps = CMakeDeps(self)
    #     deps.generate()
    #     tc = CMakeToolchain(self)
    #     tc.generate()

    def build(self):
        # cmake = CMake(self)
        # cmake.configure()
        # cmake.build()
        pass

    def package(self):
        code_dir = os.path.join(self.source_folder, "allocators")
        include_dir = os.path.join(code_dir, "include")
        copy(self, "LICENSE*",          src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        copy(self, "*CMakeLists.txt",   src=self.source_folder, dst=self.package_folder)
        copy(self, "*.hpp",             src=include_dir,        dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.ipp",             src=include_dir,        dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.h",               src=include_dir,        dst=os.path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "dd99::allocators")
        # self.cpp_info.set_property("pkg_config_name", "allocators")
        self.cpp_info.bindirs = []
        self.cpp_info.frameworkdirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.resdirs = []

    # def export_sources(self):
    #     copy(self, "*.hpp", src=self.recipe_folder, dst=self.export_sources_folder)
    #     copy(self, "*.ipp", src=self.recipe_folder, dst=self.export_sources_folder)
    #     copy(self, "*.h",   src=self.recipe_folder, dst=self.export_sources_folder)
    #     # copy(self, "CMakeLists.txt", self.recipe_folder, self.export_sources_folder)
