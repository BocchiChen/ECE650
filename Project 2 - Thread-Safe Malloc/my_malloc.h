#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

struct memory_control_block{
  size_t mem_size;
  int is_available;
  struct memory_control_block * prev_fmr;
  struct memory_control_block * next_fmr;
};
typedef struct memory_control_block mcb;

void * ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);
void * allocate_free_space(mcb * p, size_t size, mcb ** first_free_block, mcb ** last_free_block);
void * increase_total_space(size_t num_of_bytes, int sbrk_lock);
void add_free_block(mcb * ablock, mcb ** first_free_block, mcb ** last_free_block);
void delete_free_block(mcb * dblock, mcb ** first_free_block, mcb ** last_free_block);
void * bf_malloc(size_t size, mcb ** first_free_block, mcb ** last_free_block, int sbrk_lock);
void bf_free(void * ptr, mcb ** first_free_block, mcb ** last_free_block);
