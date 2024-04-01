from setuptools import setup, Extension

extensions = [
    Extension(
        "json_autocomplete.json_autocomplete",
        ["json_autocomplete/json_autocomplete.cpp"],
    )
]

setup(
    packages=["json_autocomplete"],
    ext_modules=extensions,
)
