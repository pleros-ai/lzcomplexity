import os
import re
import subprocess
import sys
import glob
import shutil
from pathlib import Path

from setuptools import Extension, setup, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.sdist import sdist as _sdist

# Try to use tomli for TOML parsing, fall back to built-in if Python 3.11+
try:
    import tomli
except ImportError:
    try:
        import tomllib as tomli
    except ImportError:
        tomli = None

# Read version from pyproject.toml
try:
    if tomli:
        with open("pyproject.toml", "rb") as f:
            pyproject = tomli.load(f)
        __version__ = pyproject["project"]["version"]
    else:
        # Simple fallback parser for Python 3.11+
        import re
        with open("pyproject.toml", "r", encoding="utf-8") as f:
            content = f.read()
            version_match = re.search(r'version\s*=\s*"([^"]+)"', content)
            if version_match:
                __version__ = version_match.group(1)
            else:
                __version__ = "0.9.13"  # Fallback version
except (FileNotFoundError, KeyError, ImportError):
    __version__ = "0.9.13"  # Fallback version

# Convert distutils Windows platform specifiers to CMake -A arguments
PLAT_TO_CMAKE = {
    "win32": "Win32",
    "win-amd64": "x64",
    "win-arm32": "ARM",
    "win-arm64": "ARM64",
}


# A CMakeExtension needs a sourcedir instead of a file list.
# The name must be the _single_ output extension from the CMake build.
# If you need multiple extensions, see scikit-build.
# Enhanced sdist command to include all necessary files
class EnhancedSDist(_sdist):
    def run(self):
        # Make sure all necessary files are included
        self.announce("Preparing source distribution")
        # Run the standard sdist
        _sdist.run(self)


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = "") -> None:
        # Find all source files
        sources = (
            glob.glob('modules/core/src/*.cpp') +
            glob.glob('modules/sa/src/*.cpp') +
            glob.glob('modules/utils/src/*.cpp') +
            glob.glob('python/src/*.cpp')
        )

        # Initialize the extension with all source files
        super().__init__(name, sources=sources)

        # Set the source directory
        self.sourcedir = os.fspath(Path(sourcedir).resolve())

        # Include directories
        self.include_dirs = [
            'modules/core/inc',
            'modules/utils/inc',
            'modules/sa/inc',
            'python/src/inc'
        ]


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        # Must be in this form due to bug in .resolve() only fixed in Python 3.10+
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        # Using this requires trailing slash for auto-detection & inclusion of
        # auxiliary "native" libs

        debug = int(os.environ.get("DEBUG", 0)) if self.debug is None else self.debug
        cfg = "Debug" if debug else "Release"

        # CMake lets you override the generator - we need to check this.
        # Can be set with Conda-Build, for example.
        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

        # Set Python_EXECUTABLE instead if you use PYBIND11_FINDPYTHON
        # EXAMPLE_VERSION_INFO shows you how to pass a value into the C++ code
        # from Python.
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={cfg}",  # not used on MSVC, but no harm
        ]
        build_args = []

        # Adding CMake arguments set as environment variable
        # (needed e.g. to build for ARM OSx on conda-forge)
        if "CMAKE_ARGS" in os.environ:
            cmake_args += [item for item in os.environ["CMAKE_ARGS"].split(" ") if item]

        # In this example, we pass in the version to C++. You might not need to.
        cmake_args += [f"-DEXAMPLE_VERSION_INFO={self.distribution.get_version()}"]

        if self.compiler.compiler_type != "msvc":
            # Using Ninja-build since it a) is available as a wheel and b)
            # multithreads automatically. MSVC would require all variables be
            # exported for Ninja to pick it up, which is a little tricky to do.
            # Users can override the generator with CMAKE_GENERATOR in CMake
            # 3.15+.
            if not cmake_generator or cmake_generator == "Ninja":
                try:
                    import ninja

                    ninja_executable_path = Path(ninja.BIN_DIR) / "ninja"
                    cmake_args += [
                        "-GNinja",
                        f"-DCMAKE_MAKE_PROGRAM:FILEPATH={ninja_executable_path}",
                    ]
                except ImportError:
                    pass

        else:
            # Single config generators are handled "normally"
            single_config = any(x in cmake_generator for x in {"NMake", "Ninja"})

            # CMake allows an arch-in-generator style for backward compatibility
            contains_arch = any(x in cmake_generator for x in {"ARM", "Win64"})

            # Specify the arch if using MSVC generator, but only if it doesn't
            # contain a backward-compatibility arch spec already in the
            # generator name.
            if not single_config and not contains_arch:
                cmake_args += ["-A", PLAT_TO_CMAKE[self.plat_name]]

            # Multi-config generators have a different way to specify configs
            if not single_config:
                cmake_args += [
                    f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}"
                ]
                build_args += ["--config", cfg]

        if sys.platform.startswith("darwin"):
            # Cross-compile support for macOS - respect ARCHFLAGS if set
            archs = re.findall(r"-arch (\S+)", os.environ.get("ARCHFLAGS", ""))
            if archs:
                cmake_args += ["-DCMAKE_OSX_ARCHITECTURES={}".format(";".join(archs))]

        # Set CMAKE_BUILD_PARALLEL_LEVEL to control the parallel build level
        # across all generators.
        if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
            # self.parallel is a Python 3 only way to set parallel jobs by hand
            # using -j in the build_ext call, not supported by pip or PyPA-build.
            if hasattr(self, "parallel") and self.parallel:
                # CMake 3.12+ only.
                build_args += [f"-j{self.parallel}"]

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)

        cmake_args += ["-DLZ_ONLY_CORE=ON", "-DBUILD_PYTHON=ON", "-DLZ_APP=OFF", "-DLZ_DISTANCE=OFF"]
        cmake_args += ["-DBUILTIN_TBB=ON", "-DFIXED_VERSION=3.12", "-DFIXED_PYTHON=ON"]

        subprocess.run(
            ["cmake", ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=build_temp, check=True
        )
        # subprocess.run(
        #     ["make"], cwd=build_temp, check=True
        # )

# Read long description from README.md
try:
    with open("README.md", "r", encoding="utf-8") as f:
        long_description = f.read()
except FileNotFoundError:
    long_description = "LempelZiv-76 complexity engine. Suited for complexity analysis of time series."


setup(
    name="lzcomplexity",
    version=__version__,
    # url="https://github.com/ZentropyUH/LempelZiv",  # Update with your actual repository URL
    author="Efren Aragon",
    author_email="efrenaragon96@gmail.com",
    license="GNU General Public License",
    description="LempelZiv-76 complexity engine",
    long_description=long_description,
    long_description_content_type="text/markdown",
    # Don't use find_packages for C++ extension
    # packages=find_packages(where="python/src"),
    # package_dir={"": "python/src"},
    classifiers=[
        "Development Status :: 4 - Beta",
        "Environment :: Console",
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "Topic :: Software Development :: Libraries",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Information Analysis"
    ],
    ext_modules=[CMakeExtension('')],
    python_requires='>=3.9',
    cmdclass={
        'build_ext': CMakeBuild,
        'sdist': EnhancedSDist,
    },
    zip_safe=False,
    # Add any Python dependencies here
    # install_requires=[
    #     'numpy>=1.20.0',
    # ],
)