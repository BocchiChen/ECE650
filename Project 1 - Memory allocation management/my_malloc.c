#include "my_malloc.h"

void * end_address = NULL;
mcb * first_free_block = NULL;
mcb * last_free_block = NULL;
size_t data_segment_size = 0;
size_t free_space_size = 0;

void * allocate_free_space(mcb * p, size_t size){
  size_t num_of_bytes = size + sizeof(mcb);
  size_t remaining_free_size = p->mem_size - num_of_bytes;
  if(remaining_free_size <= sizeof(mcb)){
    delete_free_block(p);
    free_space_size -= p->mem_size;
    return (char *)p + sizeof(mcb);
  }else{
    //split the block and use the second one as the allocated block
    size_t omem = p->mem_size;
    mcb * allocated_block = (mcb *) ((char *)p + omem - num_of_bytes);
    allocated_block->mem_size = num_of_bytes;
    allocated_block->is_available = 0;
    allocated_block->prev_fmr = NULL;
    allocated_block->next_fmr = NULL;
    p->mem_size = remaining_free_size;
    free_space_size -= num_of_bytes;
    return (char *)p + omem - size;
  }
}

void * increase_total_space(size_t num_of_bytes){
  end_address = sbrk(0);
  if(sbrk(num_of_bytes) == (void *)-1){
    return NULL;
  }
  mcb * allocated_block = (mcb *) end_address;
  allocated_block->mem_size = num_of_bytes;
  allocated_block->is_available = 0;
  allocated_block->prev_fmr = NULL;
  allocated_block->next_fmr = NULL;
  data_segment_size += num_of_bytes;
  return (char *)allocated_block + sizeof(mcb);
}

void * ff_malloc(size_t size){
  size_t num_of_bytes = size + sizeof(mcb);
  mcb * p = first_free_block;
  while(p != NULL){
    if(p->mem_size >= num_of_bytes){
      return allocate_free_space(p, size);
    }
    p = p->next_fmr;
  }
  //if none free block is available
  return increase_total_space(num_of_bytes);
}

void add_free_block(mcb * ablock){
  ablock->next_fmr = NULL;
  ablock->prev_fmr = NULL;
  if(first_free_block == NULL){
    first_free_block = ablock;
    last_free_block = first_free_block;
    return;
  }
  if(ablock < first_free_block){
    first_free_block->prev_fmr = ablock;
    ablock->next_fmr = first_free_block;
    first_free_block = ablock;
    return;
  }
  int added = 0;
  mcb * cb = first_free_block;
  while(cb != NULL){
    if(cb > ablock){
      ablock->prev_fmr = cb->prev_fmr;
      ablock->next_fmr = cb;
      cb->prev_fmr->next_fmr = ablock;
      cb->prev_fmr = ablock;
      added = 1;
      break;
    }
    cb = cb->next_fmr;
  }
  if(added == 0){
    last_free_block->next_fmr = ablock;
    ablock->prev_fmr = last_free_block;
    last_free_block = ablock;
  }
}

void delete_free_block(mcb * dblock){
  dblock->is_available = 0;
  if(dblock == last_free_block && dblock == first_free_block){
    first_free_block = NULL;
    last_free_block = NULL;
  }else if(dblock == first_free_block){
    first_free_block = dblock->next_fmr;
    dblock->next_fmr->prev_fmr = NULL;
  }else if(dblock == last_free_block){
    last_free_block = dblock->prev_fmr;
    dblock->prev_fmr->next_fmr = NULL;
  }else{
    dblock->prev_fmr->next_fmr = dblock->next_fmr;
    dblock->next_fmr->prev_fmr = dblock->prev_fmr;
  }
  dblock->prev_fmr = NULL;
  dblock->next_fmr = NULL;
}

void ff_free(void * ptr){
  if(ptr == NULL){
    return;
  }
  mcb * current_block = (mcb *) ((char *)ptr - sizeof(mcb));
  free_space_size += current_block->mem_size;
  current_block->is_available = 1;
  add_free_block(current_block);
  //merge with the later block if possible
  if((current_block->next_fmr != NULL) && ((char *)current_block + current_block->mem_size == (char *)current_block->next_fmr)){
    current_block->mem_size += current_block->next_fmr->mem_size;
    delete_free_block(current_block->next_fmr);
  }
  //merge with the former block if possible
  if((current_block->prev_fmr != NULL) && ((char *)current_block->prev_fmr + current_block->prev_fmr->mem_size == (char *)current_block)){
    current_block->prev_fmr->mem_size += current_block->mem_size;
    delete_free_block(current_block);
  }
  return;
}
 
void printList(){
  mcb * p = first_free_block;
  while(p != NULL){
    printf("chunk start address: %p, chunk size: %lu\n", p, p->mem_size);
    p = p->next_fmr;
  }
}

void * bf_malloc(size_t size){
  size_t num_of_bytes = size + sizeof(mcb);
  if(first_free_block != NULL){
    mcb * p = first_free_block;
    mcb * target = NULL;
    size_t min_gap = 0;
    while(p != NULL){
       if(p->mem_size == num_of_bytes){
         return allocate_free_space(p,size);
       }else if(p->mem_size > num_of_bytes){
         if(target == NULL || ((p->mem_size - num_of_bytes) < min_gap)){
           min_gap = p->mem_size - num_of_bytes;
           target = p;
         }
       }
       p = p->next_fmr;
    }
    if(target != NULL){
      return allocate_free_space(target,size);
    }
  }
  //if none free block is available
  return increase_total_space(num_of_bytes);
}

void bf_free(void * ptr){
  ff_free(ptr);
}

unsigned long get_data_segment_size(){
  return data_segment_size;
}

unsigned long get_data_segment_free_space_size(){
  return free_space_size;
}
