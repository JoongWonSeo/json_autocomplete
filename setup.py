from setuptools import setup, Extension
from Cython.Build import cythonize

extensions = [
    Extension(
        "json_autocomplete.json_autocomplete",
        ["json_autocomplete/json_autocomplete.pyx"],
        extra_compile_args=["-std=c++11"],
        extra_link_args=["-std=c++11"],
    )
]

setup(
    packages=["json_autocomplete"],
    ext_modules=cythonize(extensions),
)
