import os
import re
import subprocess
import sys
import glob
import sysconfig
from pathlib import Path

from setuptools import Extension, setup, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.sdist import sdist as _sdist

# version
def read_version() -> str:
    """Read version from pyproject.toml, with fallback."""
    try:
        # Python 3.11+ has tomllib built in
        if sys.version_info >= (3, 11):
            import tomllib
            with open("pyproject.toml", "rb") as f:
                return tomllib.load(f)["project"]["version"]
        else:
            # Try third-party tomli
            try:
                import tomli
                with open("pyproject.toml", "rb") as f:
                    return tomli.load(f)["project"]["version"]
            except ImportError:
                pass
    except (FileNotFoundError, KeyError):
        pass

    # Last resort: regex parse
    try:
        content = Path("pyproject.toml").read_text(encoding="utf-8")
        m = re.search(r'^version\s*=\s*"([^"]+)"', content, re.MULTILINE)
        if m:
            return m.group(1)
    except FileNotFoundError:
        pass

    return "0.10.1"

def find_python_include() -> str:
    """
    Reliably find Python.h across venvs, cibuildwheel, and manylinux.
    Tries multiple strategies in order.
    """
    candidates = [
        # 1. base prefix (most reliable — bypasses the venv layer)
        sysconfig.get_path("include", vars={"installed_base": sys.base_prefix}),

        # 2. standard scheme include
        sysconfig.get_path("include"),

        # 3. platinclude (some distros put it here)
        sysconfig.get_path("platinclude"),

        # 4. manual construction from base_prefix
        str(Path(sys.base_prefix) / "include" / f"python{sys.version_info.major}.{sys.version_info.minor}"),
        str(Path(sys.base_prefix) / "include" / f"python{sys.version_info.major}.{sys.version_info.minor}m"),

        # 5. macOS framework path
        str(Path(sys.base_prefix) / "Headers"),
    ]

    for path in candidates:
        if path and (Path(path) / "Python.h").exists():
            print(f"── Found Python.h in: {path}")
            return path

    # Last resort: ask the compiler via python3-config
    try:
        import subprocess
        result = subprocess.run(
            [sys.executable, "-c",
             "import sysconfig; print(sysconfig.get_path('include', vars={'installed_base': __import__('sys').base_prefix}))"],
            capture_output=True, text=True, check=True
        )
        path = result.stdout.strip()
        if path and (Path(path) / "Python.h").exists():
            return path
    except Exception:
        pass

    raise RuntimeError(
        f"Could not find Python.h. Tried:\n" +
        "\n".join(f"  {p}" for p in candidates if p)
    )


def find_python_library() -> str:
    """Find the Python library for linking (needed on some platforms)."""
    libdir = sysconfig.get_config_var("LIBDIR") or ""
    ldlibrary = sysconfig.get_config_var("LDLIBRARY") or ""
    if libdir and ldlibrary:
        candidate = str(Path(libdir) / ldlibrary)
        if Path(candidate).exists():
            return candidate
    return ""

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

        python_executable = sys.executable
        python_include    = find_python_include()   # robust finder above
        python_library    = find_python_library()

        cmake_args = [
            # Put the compiled .so/.pyd directly where setuptools expects it
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            # Force CMake to use exactly the Python running this script
            f"-DPYTHON_EXECUTABLE={python_executable}",          # old FindPython
            f"-DPython_EXECUTABLE={python_executable}",          # new FindPython
            f"-DPython3_EXECUTABLE={python_executable}",         # FindPython3

            # Point directly at the headers 
            f"-DPYTHON_INCLUDE_DIR={python_include}",
            f"-DPython_INCLUDE_DIR={python_include}",
            f"-DPython3_INCLUDE_DIR={python_include}",
            
            # Prevent CMake from searching elsewhere
            "-DPYTHON_FOUND=TRUE",
            "-DPython_FIND_VIRTUALENV=FIRST",                  # only look in virtualenv
            "-DPython3_FIND_VIRTUALENV=FIRST",

            f"-DCMAKE_BUILD_TYPE={cfg}",
            # Project-specific flags
            "-DLZ_ONLY_CORE=ON",
            "-DBUILD_PYTHON=ON",
            "-DLZ_APP=OFF",
            "-DLZ_DISTANCE=OFF"
        ]

        if python_library:
            cmake_args += [
                f"-DPYTHON_LIBRARY={python_library}",
                f"-DPython_LIBRARY={python_library}",
                f"-DPython3_LIBRARY={python_library}",
            ]

        build_args = []

        # CMake lets you override the generator - we need to check this.
        # Can be set with Conda-Build, for example.
        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

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
    version=read_version(),
    # url="https://github.com/ZentropyUH/LempelZiv",  # Update with your actual repository URL
    author="Efren Aragon",
    author_email="support@pleros.ai",
    license="MIT License",
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
    python_requires='>=3.9',
    ext_modules=[CMakeExtension("lzcomplexity")],  # must match your nanobind module name
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    # Add any Python dependencies here
    # install_requires=[
    #     'numpy>=1.20.0',
    # ],
)