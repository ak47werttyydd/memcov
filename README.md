Quickly allow you to monitor/control in-memory coverage for your C++-implemented-Python-binded libraries.

This version is designed for running memcov in Python **multiprocessing** package. Because processes don't share memory, so this version settles it by saving and loading coverage information in processes. 

# How to use this repository to track Apache TVM's coverage?
1. Download **memcov** by `git clone https://github.com/ak47werttyydd/memcov.git`
2. Setting building requirements for installing TVM, please referring to https://tvm.apache.org/docs/install/from_source.html
3. Download **tzer** by `git clone https://github.com/ise-uiuc/tzer.git`. Follow `tzer/tvm_cov_patch/build_tvm.sh`[https://github.com/ise-uiuc/tzer/blob/main/tvm_cov_patch/build_tvm.sh] to build TVM with coverage tracing until complete the 10th line
![image](https://github.com/ak47werttyydd/memcov/assets/49711033/1d7a64a9-240e-40d7-b708-2bf8a1d68357)
4. rewrite `path_to_your/tvm/cmake/modules/contrib/Memcov.cmake` as
   ```cmake
   if(USE_COV)
    #Please enter Path to your local memcov (where you git clone this memcov)
    set(memcov_SOURCE_DIR "path/to/local/memcov")

    #The Following Codes Don't Have to Modify
    # Add the local memcov directory to the build
    add_subdirectory(${memcov_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/memcov)

    # Append memcov to the linker libraries
    list(APPEND TVM_LINKER_LIBS memcov)
    list(APPEND TVM_RUNTIME_LINKER_LIBS memcov)
endif()
5. modify python interface at `path_to_your/tvm/python/tvm/contrib/coverage.py` as the following Python interface (scroll down and you can find it)
6. Proceed with line 11th in `tzer/tvm_cov_patch/build_tvm.sh`[https://github.com/ise-uiuc/tzer/blob/main/tvm_cov_patch/build_tvm.sh] to build Apache TVM until line 23th
7. Write your Python Script to compile and run models on Apache TVM. Using **multiprocessing** to raise a process to compile and run a model

```python
from tvm.contrib import coverage

print(coverage.get_now()) # Current visited # of CFG edges
print(coverage.get_total()) # Total number of # of CFG edges

#pseudocode
def run_and_compile_a_model():

    coverage.push() # store current coverage snapshot to a stack and reset it to empty (useful for multi-process scenario)

    '''compile and run model ....'''

    coverage.pop() # merge previous coverage with current coverage

    '''Then save coverage information by the following code'''
    print("------------serialize_mem_coverage start-----------------")
    size = ctypes.c_size_t()  # creates an instance of ctypes.c_size_t to accept size of serialized_coverage
    serialized_coverage_ptr=coverage.serialize_mem_coverage(ctypes.byref(size)) #get serizlized_coverage's pointer and size
    serialized_coverage=ctypes.string_at(serialized_coverage_ptr,size.value) #convert to python bytes
    print("------------serialized_coverage get!----------------")
    #please modify the following code to save serialized_coverage to your local file
    current_dir=os.path.dirname(os.path.realpath(__file__))
    filename=os.path.join(current_dir,"mem_coverage.bin") #write serialized coverage into current directory with name "mem_coverage.bin"
    with open(filename, 'wb') as file:
        file.write(serialized_coverage)
    print("------------serialize_mem_coverage finish-----------------")



while model in models:

    '''raise a process to run_and_compile_a_model(model,other_arguments)'''

    '''Until finishing previous process, Load coverage information in the previous process by the following code'''
    print("------------deserialize_mem_coverage start-----------------")
        current_dir = os.path.dirname(os.path.realpath(__file__))
        filename = os.path.join(current_dir, "mem_coverage.bin")
        serialized_coverage=read_bytes_from_file(filename)
        # print("serialized_coverage is ",serialized_coverage) #read done
        serialized_coverage_ptr = ( ctypes.c_uint8 * len(serialized_coverage) ).from_buffer_copy(serialized_coverage) #create an array from python bytes' object
        coverage.deserialize_mem_coverage(serialized_coverage_ptr,len(serialized_coverage))
        print("------------deserialize_mem_coverage finish-----------------")
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
#save (or serialize) mem_coverage
_LIB.serialize_mem_coverage.restype = ctypes.POINTER(ctypes.c_uint8) #match the return type uint8*
_LIB.serialize_mem_coverage.argtypes = [ ctypes.POINTER( ctypes.c_size_t ) ] #math arguments type size_t*
serialize_mem_coverage = _LIB.serialize_mem_coverage

#in your Python script where compiling and running model on Apache TVM, put the following code at the end of a process
'''
    from tvm.contrib import coverage
    print("------------serialize_mem_coverage start-----------------")
    size = ctypes.c_size_t()  # creates an instance of ctypes.c_size_t to accept size of serialized_coverage
    serialized_coverage_ptr=coverage.serialize_mem_coverage(ctypes.byref(size)) #get serizlized_coverage's pointer and size
    serialized_coverage=ctypes.string_at(serialized_coverage_ptr,size.value) #convert to python bytes
    print("------------serialized_coverage get!----------------")
    #please modify the following code to save serialized_coverage to your local file
    current_dir=os.path.dirname(os.path.realpath(__file__))
    filename=os.path.join(current_dir,"mem_coverage.bin") #write serialized coverage into current directory with name "mem_coverage.bin"
    with open(filename, 'wb') as file:
        file.write(serialized_coverage)
    print("------------serialize_mem_coverage finish-----------------")
'''

#load the serialized_coverage to mem_coverage
deserialize_mem_coverage = _LIB.deserialize_mem_coverage
deserialize_mem_coverage.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
deserialize_mem_coverage.restype = None

#in your Python script where compiling and running model on Apache TVM, put the following code after finishing a previous process or at the beginning of a new process
'''
        from tvm.contrib import coverage
        print("------------deserialize_mem_coverage start-----------------")
        #please modify the following code to load serialized_coverage from the local file "mem_coverage.bin"
        current_dir = os.path.dirname(os.path.realpath(__file__))
        filename = os.path.join(current_dir, "mem_coverage.bin")
        with open(filename, 'rb') as file:
            serialized_coverage = file.read()
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
