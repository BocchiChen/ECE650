#include "my_malloc.h"

mcb * first_free_block_lock = NULL;
mcb * last_free_block_lock = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread mcb * first_free_block_nolock = NULL;
__thread mcb * last_free_block_nolock = NULL;

void * ts_malloc_lock(size_t size){
  pthread_mutex_lock(&lock);
  void * p = bf_malloc(size,&first_free_block_lock,&last_free_block_lock,0);
  pthread_mutex_unlock(&lock);
  return p;
}

void ts_free_lock(void *ptr){
  pthread_mutex_lock(&lock);
  bf_free(ptr,&first_free_block_lock,&last_free_block_lock);
  pthread_mutex_unlock(&lock);
}

void * ts_malloc_nolock(size_t size){
  return bf_malloc(size,&first_free_block_nolock,&last_free_block_nolock,1);
}

void ts_free_nolock(void * ptr){
  bf_free(ptr,&first_free_block_nolock,&last_free_block_nolock);
}

void * allocate_free_space(mcb * p, size_t size, mcb ** first_free_block, mcb ** last_free_block){
  size_t num_of_bytes = size + sizeof(mcb);
  size_t remaining_free_size = p->mem_size - num_of_bytes;
  if(remaining_free_size <= sizeof(mcb)){
    delete_free_block(p,first_free_block,last_free_block);
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
    return (char *)p + omem - size;
  }
}

void * increase_total_space(size_t num_of_bytes, int sbrk_lock){
  mcb * allocated_block = NULL;
  if(sbrk_lock == 0){
    allocated_block = sbrk(num_of_bytes);
  }else{
    pthread_mutex_lock(&lock);
    allocated_block = sbrk(num_of_bytes);
    pthread_mutex_unlock(&lock);
  }
  allocated_block->mem_size = num_of_bytes;
  allocated_block->is_available = 0;
  allocated_block->prev_fmr = NULL;
  allocated_block->next_fmr = NULL;
  return (char *)allocated_block + sizeof(mcb);
}

void add_free_block(mcb * ablock, mcb ** first_free_block, mcb ** last_free_block){
  ablock->next_fmr = NULL;
  ablock->prev_fmr = NULL;
  if(*first_free_block == NULL){
    *first_free_block = ablock;
    *last_free_block = *first_free_block;
    return;
  }
  if(ablock < *first_free_block){
    (*first_free_block)->prev_fmr = ablock;
    ablock->next_fmr = *first_free_block;
    *first_free_block = ablock;
    return;
  }
  int added = 0;
  mcb * cb = *first_free_block;
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
    (*last_free_block)->next_fmr = ablock;
    ablock->prev_fmr = *last_free_block;
    *last_free_block = ablock;
  }
}

void delete_free_block(mcb * dblock, mcb ** first_free_block, mcb ** last_free_block){
  dblock->is_available = 0;
  if(dblock == *last_free_block && dblock == *first_free_block){
    *first_free_block = NULL;
    *last_free_block = NULL;
  }else if(dblock == *first_free_block){
    *first_free_block = dblock->next_fmr;
    dblock->next_fmr->prev_fmr = NULL;
  }else if(dblock == *last_free_block){
    *last_free_block = dblock->prev_fmr;
    dblock->prev_fmr->next_fmr = NULL;
  }else{
    dblock->prev_fmr->next_fmr = dblock->next_fmr;
    dblock->next_fmr->prev_fmr = dblock->prev_fmr;
  }
  dblock->prev_fmr = NULL;
  dblock->next_fmr = NULL;
}
 
void * bf_malloc(size_t size, mcb ** first_free_block, mcb ** last_free_block, int sbrk_lock){
  size_t num_of_bytes = size + sizeof(mcb);
  if(*first_free_block != NULL){
    mcb * p = *first_free_block;
    mcb * target = NULL;
    size_t min_gap = 0;
    while(p != NULL){
       if(p->mem_size == num_of_bytes){
         return allocate_free_space(p,size,first_free_block,last_free_block);
       }else if(p->mem_size > num_of_bytes){
         if(target == NULL || ((p->mem_size - num_of_bytes) < min_gap)){
           min_gap = p->mem_size - num_of_bytes;
           target = p;
         }
       }
       p = p->next_fmr;
    }
    if(target != NULL){
      return allocate_free_space(target,size,first_free_block,last_free_block);
    }
  }
  //if none free block is available
  return increase_total_space(num_of_bytes,sbrk_lock);
}

void bf_free(void * ptr, mcb ** first_free_block, mcb ** last_free_block){
  if(ptr == NULL){
    return;
  }
  mcb * current_block = (mcb *) ((char *)ptr - sizeof(mcb));
  current_block->is_available = 1;
  add_free_block(current_block,first_free_block, last_free_block);
  //merge with the later block if possible
  if((current_block->next_fmr != NULL) && ((char *)current_block + current_block->mem_size == (char *)current_block->next_fmr)){
    current_block->mem_size += current_block->next_fmr->mem_size;
    delete_free_block(current_block->next_fmr,first_free_block, last_free_block);
  }
  //merge with the former block if possible
  if((current_block->prev_fmr != NULL) && ((char *)current_block->prev_fmr + current_block->prev_fmr->mem_size == (char *)current_block)){
    current_block->prev_fmr->mem_size += current_block->mem_size;
    delete_free_block(current_block,first_free_block, last_free_block);
  }
  return;
}

