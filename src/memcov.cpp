#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include <vector>

#include "coverage_struct.hpp"

mcov_t mem_coverage;
uint32_t mcov_total;

constexpr size_t mcov_max_stack_size = 8;
static mcov_t mcov_stack[mcov_max_stack_size];
static size_t mcov_stack_size = 0;

static mcov_t mcov_make_coverage() noexcept {
    mcov_t new_cov;
    new_cov.storage_size = (mcov_total + 7) / 8;
    new_cov.storage = (uint8_t*)std::malloc(mem_coverage.storage_size * sizeof(uint8_t));
    std::memset(new_cov.storage, 0, new_cov.storage_size);
    return new_cov;
}

extern "C" {

uint32_t get_count(const mcov_t& mcov_tested) noexcept{
    uint32_t count=0;
    for(uint32_t i=0; i < mcov_tested.storage_size ; i++){
        count += __builtin_popcount(mcov_tested.storage[i]);
    }
    return count;
}

// Reset current coverage and push it to a stack.
void mcov_push_coverage() noexcept {
    if (mcov_stack_size == mcov_max_stack_size) {
        printf("mcov_push_coverage: stack overflow! max coverage stack size is %li\n", mcov_max_stack_size);
        abort();
    }
    mcov_stack[mcov_stack_size] = mcov_make_coverage();

    uint32_t count = get_count(mem_coverage);
    printf("In mcov_push_coverage(), BEFORE swapping mem_coverage with mcov_stack[top], mem_coverage's coverage is %u\n",count);  //mem_coverage.storage is 0

    count = get_count(mcov_stack[mcov_stack_size]);
    printf("In mcov_push_coverage(), BEFORE swapping mem_coverage with mcov_stack[top], mcov_stack[top]'s coverage is %u\n",count);

    std::swap(mcov_stack[mcov_stack_size], mem_coverage);
    ++mcov_stack_size; //mcov_stack_size points to the immediate blank above head element

    count = get_count(mem_coverage);
    printf("In mcov_push_coverage(), AFTER swapping mem_coverage with mcov_stack[previous top], mem_coverage's coverage is %u\n",count);

    count = get_count(mcov_stack[mcov_stack_size-1]);
    printf("In mcov_push_coverage(), AFTER swapping mem_coverage with mcov_stack[previous top], mcov_stack[previous top]'s coverage is %u\n",count);
}

// Pop last coverage from the stack and merge it into current coverage.
void mcov_pop_coverage() noexcept {
    if (0 == mcov_stack_size) {
        printf("You cannot pop an empty stack!");
        abort();
    }

    mcov_t last_cov = mcov_stack[mcov_stack_size - 1]; //shallow copy

    uint32_t count1 = get_count(mem_coverage);
    printf("In mcov_pop_coverage(), BEFORE merging, mem_coverage's coverage is %u\n",count1);

    count1 = get_count(last_cov);
    printf("In mcov_pop_coverage(), BEFORE merging, mcov_stack[top-1]'s coverage is %u\n",count1);

    uint32_t count = 0;
    // merge it into current cov.
    for (size_t i = 0; i < mem_coverage.storage_size; ++i) {
        mem_coverage.storage[i] |= last_cov.storage[i];
        count += __builtin_popcount(mem_coverage.storage[i]);
    }
    mem_coverage.now = count;
    free(last_cov.storage);
    --mcov_stack_size;

    count = get_count(mem_coverage);
    printf("In mcov_pop_coverage(), AFTER merging, mem_coverage's coverage is %u\n",count);

}

void mcov_set_now(uint32_t n) noexcept {
    mem_coverage.now = n;  //no use, because the storage doesn't change, which maintains the details of coverage
}


//default arg is mem_coverage
//std::vector<uint8_t> serialize_mem_coverage() {
//    // Calculate total size needed: size of 'now', 'storage_size', and the storage array
//    size_t total_size = sizeof(mem_coverage.now) + sizeof(mem_coverage.storage_size) + mem_coverage.storage_size;
//    std::vector<uint8_t> buffer(total_size);
//
//    // Pointer to the current position in the buffer
//    uint8_t* ptr = buffer.data();
//
//    // Copy 'now' and 'storage_size' to the buffer
//    std::memcpy(ptr, &mem_coverage.now, sizeof(mem_coverage.now));
//    printf("Copying mem_coverage.now is done");
//    ptr += sizeof(mem_coverage.now);
//    std::memcpy(ptr, &mem_coverage.storage_size, sizeof(mem_coverage.storage_size));
//    ptr += sizeof(mem_coverage.storage_size);
//    printf("Copying mem_coverage.storage_size is done");
//
//    // Copy 'storage' data
//    std::memcpy(ptr, mem_coverage.storage, mem_coverage.storage_size);
//    printf("Copying mem_coverage.storage is done");
//
//    return buffer;  //move copy buffer to serialize_mem_coverage.
//}

//size is an output pointer to tell python the size of data
uint8_t* serialize_mem_coverage(size_t* size) {
    *size = sizeof(mem_coverage.now) + sizeof(mem_coverage.storage_size) + mem_coverage.storage_size;
    uint8_t* buffer = (uint8_t*)malloc(*size);
    uint8_t* ptr = buffer;

    // Copy 'now' and 'storage_size' to the buffer
    memcpy(ptr, &mem_coverage.now, sizeof(mem_coverage.now));
    ptr += sizeof(mem_coverage.now);
    memcpy(ptr, &mem_coverage.storage_size, sizeof(mem_coverage.storage_size));
    ptr += sizeof(mem_coverage.storage_size);

    // Copy 'storage' data
    memcpy(ptr, mem_coverage.storage, mem_coverage.storage_size);

    return buffer;
}


void deserialize_mem_coverage(uint8_t* buffer, size_t size) {

    //set mem_coverage.now
    std::memcpy(&mem_coverage.now,buffer,sizeof(mem_coverage.now));
    printf("Reading mem_coverage.now is done\n");

    //set mem_coverage.storage_size
    buffer+=sizeof(mem_coverage.now);
    std::memcpy(&mem_coverage.storage_size,buffer,sizeof(mem_coverage.storage_size));
    printf("mem_coverage.storage_size is %u\n",mem_coverage.storage_size);

    //set mem_coverage.storage
    // Free existing memory if already allocated
    if (mem_coverage.storage != NULL) {
        free(mem_coverage.storage);
    }
    //    mem_coverage.storage = (uint8_t*)malloc(mem_coverage.storage_size * sizeof(uint8_t));
    mem_coverage.storage = (uint8_t*)calloc(mem_coverage.storage_size, sizeof(uint8_t)); //allocate and initialize memory to 0
    buffer+=sizeof(mem_coverage.storage_size);
    std::memcpy(mem_coverage.storage,buffer,mem_coverage.storage_size);
    printf("Reading mem_coverage.storage is done\n");
}


void mcov_copy_hitmap(char* ptr) noexcept {
    std::memcpy(ptr, mem_coverage.storage, mem_coverage.storage_size);
}

void mcov_set_hitmap(char* ptr) noexcept {
    std::memcpy(mem_coverage.storage, ptr, mem_coverage.storage_size);
}

uint32_t mcov_get_now() noexcept {
    return mem_coverage.now;
}


uint32_t mcov_get_total() noexcept {
    return mcov_total;
}

void mcov_reset() noexcept {
    mem_coverage.now = 0;
    std::memset(mem_coverage.storage, 0, mem_coverage.storage_size);
}

}
