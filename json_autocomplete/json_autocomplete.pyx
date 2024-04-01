# distutils: language = c++
# distutils: sources = json_autocomplete/json.cpp
# cython: language_level=3

from libcpp.string cimport string

# Declare the C++ function
cdef extern from "json.h":
    string json_autocomplete_cpp(string prefix)

# Declare the Python function
def json_autocomplete(prefix: str) -> str:
    return json_autocomplete_cpp(prefix.encode('UTF-8')).decode('UTF-8')