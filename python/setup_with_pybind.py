# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.7.0"

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

ext_modules = [
    Pybind11Extension(
        "python_example",
        ["src/main.cpp"],
        # Example: passing in the version to the compiled code
        define_macros=[("VERSION_INFO", __version__)],
    ),
]

setup(
   name="lz",
    version=__version__,
   #  url="https://github.com/pybind/python_example",
    author="Efren Aragon",
    author_email="efrenaragon96@gmail.com",
    license="GNU General Public License",
    description="The Lempel-Ziv (lz76) complexity",
    long_description="The Lempel-Ziv (lz76) complexity",
    long_description_content_type="text/markdown",
    # packages=setuptools.find_packages("src"),
    package_dir={"": "src"},
    classifiers=[
         'Development Status :: Beta',
         'Environment :: Console',
         "Programming Language :: Python :: 3",
         "Operating System :: POSIX :: Linux",
         "Operating System :: MacOS :: MacOS X",
         "Operating System :: Microsoft :: Windows",
         'Programming Language :: C++',
         'Topic :: Software Development :: Libraries',
    ],
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    cmdclass={"build_ext": build_ext},
    python_requires='>=3.6',
    zip_safe=False
)