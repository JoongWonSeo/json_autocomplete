from setuptools import setup, Extension
from Cython.Build import cythonize

extensions = [
    Extension(
        "json_autocomplete.json_autocomplete",
        ["json_autocomplete/json_autocomplete.pyx"],
    )
]

setup(
    packages=["json_autocomplete"],
    ext_modules=cythonize(extensions),
)
