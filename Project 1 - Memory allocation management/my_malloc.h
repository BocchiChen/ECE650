#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct memory_control_block{
  size_t mem_size;
  int is_available;
  struct memory_control_block * prev_fmr;
  struct memory_control_block * next_fmr;
};
typedef struct memory_control_block mcb;

void address_init();
void * allocate_free_space(mcb * p, size_t size);
void * increase_total_space(size_t num_of_bytes);
void * ff_malloc(size_t size);
void add_free_block(mcb * ablock);
void delete_free_block(mcb * dblock);
void ff_free(void * ptr);
void printList();
void * bf_malloc(size_t size);
void bf_free(void * ptr);
unsigned long get_data_segment_size();
unsigned long get_data_segment_free_space_size();
