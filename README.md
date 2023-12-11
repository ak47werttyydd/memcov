Quickly allow you to monitor/control in-memory coverage for your C++-implemented-Python-binded libraries.
This version is designed for running memcov in Python **multiprocessing** package. Because processes don't share memory, so this version settles it by saving and loading coverage information in processes

## **\[C++\]** Add Coverage Tracing During Library Compilation

```cmake
INCLUDE(FetchContent)
FetchContent_Declare(
    memcov
    GIT_REPOSITORY https://github.com/ganler/memcov.git
)

FetchContent_GetProperties(memcov)

if(NOT memcov_POPULATED)
    FetchContent_Populate(memcov)
    ADD_SUBDIRECTORY(${memcov_SOURCE_DIR} ${memcov_BINARY_DIR})
endif()

# Allow coverage tracing to specific files!
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/path/to/what.cpp 
    PROPERTIES COMPILE_OPTIONS
    -fsanitize-coverage=edge,trace-pc-guard)

# Include it to your target!
TARGET_LINK_LIBRARIES(YOUR_TARGET PRIVATE memcov)
```

## **\[Python\]** Add Interface to Operate/Monitor Coverage

```python
from tvm._ffi.base import _LIB
import ctypes

# Because `tvm.contrib.coverage.now` relies on tvm's registry function, so after 
# calling `reset`, the coverage will not be ZERO (but very small, e.g., 6).
reset = _LIB.mcov_reset

push = _LIB.mcov_push_coverage
pop = _LIB.mcov_pop_coverage

get_total = _LIB.mcov_get_total
get_now = _LIB.mcov_get_now

set_now = _LIB.mcov_set_now

'''saving mem_coverage of a process and reload the mem_coverage across processes'''
#save (serialize) mem_coverage
_LIB.serialize_mem_coverage.restype = ctypes.POINTER(ctypes.c_uint8) #match the return type uint8*
_LIB.serialize_mem_coverage.argtypes = [ ctypes.POINTER( ctypes.c_size_t ) ] #math arguments type size_t*
serialize_mem_coverage = _LIB.serialize_mem_coverage

#in your Python script, at the end of a process
'''
    from tvm.contrib import coverage
    print("------------serialize_mem_coverage start-----------------")
    size = ctypes.c_size_t()  # creates an instance of ctypes.c_size_t to accept size of serialized_coverage
    serialized_coverage_ptr=coverage.serialize_mem_coverage(ctypes.byref(size)) #get serizlized_coverage's pointer and size
    serialized_coverage=ctypes.string_at(serialized_coverage_ptr,size.value) #convert to python bytes
    print("------------serialized_coverage get!----------------")
    #please modify the following code to save serialized_coverage to a local file
    current_dir=os.path.dirname(os.path.realpath(__file__))
    filename=os.path.join(current_dir,"mem_coverage.bin")
    write_bytes_to_file(filename,serialized_coverage)
    print("------------serialize_mem_coverage finish-----------------")
'''

#load the serialized_coverage to mem_coverage
deserialize_mem_coverage = _LIB.deserialize_mem_coverage
deserialize_mem_coverage.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
deserialize_mem_coverage.restype = None

#in your python script, after a previous process or at the beginning of a new process
'''
        from tvm.contrib import coverage
        print("------------deserialize_mem_coverage start-----------------")
        #please modify the following code to load serialized_coverage from the local file where you save serialized_coverage
        current_dir = os.path.dirname(os.path.realpath(__file__))
        filename = os.path.join(current_dir, "mem_coverage.bin")
        serialized_coverage=read_bytes_from_file(filename)
        # print("serialized_coverage is ",serialized_coverage) #read done
        serialized_coverage_ptr = ( ctypes.c_uint8 * len(serialized_coverage) ).from_buffer_copy(serialized_coverage) #create an array from python bytes' object
        coverage.deserialize_mem_coverage(serialized_coverage_ptr,len(serialized_coverage)) 
        print("------------deserialize_mem_coverage finish-----------------")
'''

_char_array = ctypes.c_char * get_total()

def get_hitmap():
    hitmap_buffer = bytearray(get_total())
    _LIB.mcov_copy_hitmap(_char_array.from_buffer(hitmap_buffer))
    return hitmap_buffer

def set_hitmap(data):
    assert len(data) == get_total()
    _LIB.mcov_set_hitmap(_char_array.from_buffer(data))

```
