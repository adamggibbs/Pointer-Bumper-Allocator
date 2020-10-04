// ==============================================================================
/**
 * pb-alloc.c
 *
 * A _pointer-bumping_ heap allocator.  This allocator *does not re-use* freed
 * blocks.  It uses _pointer bumping_ to expand the heap with each allocation.
 **/
// ==============================================================================



// ==============================================================================
// INCLUDES

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "safeio.h"
// ==============================================================================



// ==============================================================================
// MACRO CONSTANTS AND FUNCTIONS

/** The system's page size. */
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

/**
 * Macros to easily calculate the number of bytes for larger scales (e.g., kilo,
 * mega, gigabytes).
 */
#define KB(size)  ((size_t)size * 1024)
#define MB(size)  (KB(size) * 1024)
#define GB(size)  (MB(size) * 1024)

/** The virtual address space reserved for the heap. */
#define HEAP_SIZE GB(2)
// ==============================================================================


// ==============================================================================
// TYPES AND STRUCTURES

/** A header for each block's metadata. */
typedef struct header {

  /** The size of the useful portion of the block, in bytes. */
  size_t size;
  
} header_s;
// ==============================================================================



// ==============================================================================
// GLOBALS

/** The address of the next available byte in the heap region. */
static intptr_t free_addr  = 0;

/** The beginning of the heap. */
static intptr_t start_addr = 0;

/** The end of the heap. */
static intptr_t end_addr   = 0;
// ==============================================================================



// ==============================================================================
/**
 * The initialization method.  If this is the first use of the heap, initialize it.
 */

void init () {

  // Only do anything if there is no heap region (i.e., first time called).
  if (start_addr == 0) {

    DEBUG("Trying to initialize");
    
    // Allocate virtual address space in which the heap will reside. Make it
    // un-shared and not backed by any file (_anonymous_ space).  A failure to
    // map this space is fatal.
    void* heap = mmap(NULL,
		      HEAP_SIZE,
		      PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS,
		      -1,
		      0);
    if (heap == MAP_FAILED) {
      ERROR("Could not mmap() heap region");
    }

    // Hold onto the boundaries of the heap as a whole.
    start_addr = (intptr_t)heap;
    end_addr   = start_addr + HEAP_SIZE;
    free_addr  = start_addr;

    // DEBUG: Emit a message to indicate that this allocator is being called.
    DEBUG("bp-alloc initialized");

  }

} // init ()
// ==============================================================================


// ==============================================================================
/**
 * Allocate and return `size` bytes of heap space.  Expand into the heap region
 * via _pointer bumping_.
 *
 * \param size The number of bytes to allocate.

 * \return A pointer to the allocated block, if successful; `NULL` if
 *         unsuccessful.
 */
void* malloc (size_t size) {
  //initialize the heap if there is no heap region yet
  init();
  
  // if requested size is 0, return nothing because there is nothing to do
  if (size == 0) {
    return NULL;
  }

  // pad the address by the following:
  // if there are less than 8 bytes until next double word alignment
  // that means we can't fit a header before the next double word alignment
  // so then shift the free_addr pointer forward 16 bytes and align it to the next
  // double word alignment
  // else align the free_addr pointer to the next double word alignment
  // in both cases subtract 8 bytes from the double word alignment at the end
  // in order to line up the header so the block is created at the
  // double word alignment
  if( free_addr % 16 < 8){
    //free_addr = free_addr + 16 + (16 - free_addr % 16) - 8;
    free_addr = free_addr + 24 - (free_addr % 16);
  } else {
    //free_addr = free_addr + (16 - free_addr % 16) - 8;
    free_addr = free_addr + 8 - (free_addr % 16);
  }
  
  // the total allocated block size will be:
  // the requested size plus the size of its metadata header
  // store this value in total_size
  size_t    total_size = size + sizeof(header_s);
  
  // place the pointer for the header at the next free address in
  // the virtual address space which is stored in free_addr
  header_s* header_ptr = (header_s*)free_addr;

  // place the pointer for the allocated block after the header
  // this address will be at the next free address plus whatever
  // the size of the header is
  void*     block_ptr  = (void*)(free_addr + sizeof(header_s));

  // shift free_addr pointer down the heap total_size bytes
  // to mark where the new free_addr is and store this new free_addr
  intptr_t new_free_addr = free_addr + total_size;

  // if new_free_addr is beyond the end of the heap, return null
  // to signify that the heap is full when the next allocation is attempted
  // else make new_free_addr the new free_addr 
  if (new_free_addr > end_addr) {

    return NULL;

  } else {

    free_addr = new_free_addr;

  }
  
  // store the size of the allocated block in its header
  header_ptr->size = size;

  //return the pointer to the allocated block
  return block_ptr;

} // malloc()
// ==============================================================================



// ==============================================================================
/**
 * Deallocate a given block on the heap.  Add the given block (if any) to the
 * free list.
 *
 * \param ptr A pointer to the block to be deallocated.
 */
void free (void* ptr) {

  DEBUG("free(): ", (intptr_t)ptr);

} // free()
// ==============================================================================



// ==============================================================================
/**
 * Allocate a block of `nmemb * size` bytes on the heap, zeroing its contents.
 *
 * \param nmemb The number of elements in the new block.
 * \param size  The size, in bytes, of each of the `nmemb` elements.
 * \return      A pointer to the newly allocated and zeroed block, if successful;
 *              `NULL` if unsuccessful.
 */
void* calloc (size_t nmemb, size_t size) {

  // Allocate a block of the requested size.
  size_t block_size = nmemb * size;
  void*  block_ptr  = malloc(block_size);

  // If the allocation succeeded, clear the entire block.
  if (block_ptr != NULL) {
    memset(block_ptr, 0, block_size);
  }

  return block_ptr;
  
} // calloc ()
// ==============================================================================



// ==============================================================================
/**
 * Update the given block at `ptr` to take on the given `size`.  Here, if `size`
 * fits within the given block, then the block is returned unchanged.  If the
 * `size` is an increase for the block, then a new and larger block is
 * allocated, and the data from the old block is copied, the old block freed,
 * and the new block returned.
 *
 * \param ptr  The block to be assigned a new size.
 * \param size The new size that the block should assume.
 * \return     A pointer to the resultant block, which may be `ptr` itself, or
 *             may be a newly allocated block.
 */
void* realloc (void* ptr, size_t size) {

  // if there is no given block, then simply return
  // a new block of given size using malloc()
  if (ptr == NULL) {
    return malloc(size);
  }

  // if the requested size is 0, then free the current block as
  // it is no longer needed. Return NULL to signify a block of size 0.
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  
  // find the header of the old block by taking the address of the block and
  // shifting backwards in the address space the number of bytes equivalent to
  // the size of the header
  header_s* old_header = (header_s*)((intptr_t)ptr - sizeof(header_s));
  // with a pointer to the header, store the size of the old block which
  // is stored in the header
  size_t    old_size   = old_header->size;

  // if the requested size is less than the old size, then the old block
  // is sufficient and the old pointer can be returned
  if (size <= old_size) {
    return ptr;
  }
  
  // if the requested size is more than the size of the old block then a new
  // block must be allocated. Allocate a new block using malloc()
  void* new_ptr = malloc(size);

  // as long as the newly allocated block of the requested size was allocated successfully,
  // copy the contents of the old block to the new block and free the current pointer
  // to the old block
  if (new_ptr != NULL) {
    memcpy(new_ptr, ptr, old_size);
    free(ptr);
  }

  // return the pointer to the new block with contents copied over
  return new_ptr;
  
} // realloc()
// ==============================================================================



#if defined (ALLOC_MAIN)
// ==============================================================================
/**
 * The entry point if this code is compiled as a standalone program for testing
 * purposes.
 */
int main () {

  // Allocate a few blocks, then free them.
  void* x = malloc(16);
  void* y = malloc(64);
  void* z = malloc(32);

  free(z);
  free(y);
  free(x);

  return 0;
  
} // main()
// ==============================================================================
#endif
