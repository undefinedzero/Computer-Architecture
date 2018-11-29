/*
 * cache.c
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "cache.h"
#include "main.h"

/* cache configuration parameters */
static int cache_split = 0;
static int cache_usize = DEFAULT_CACHE_SIZE;
static int cache_isize = DEFAULT_CACHE_SIZE; 
static int cache_dsize = DEFAULT_CACHE_SIZE;
static int cache_block_size = DEFAULT_CACHE_BLOCK_SIZE;
static int words_per_block = DEFAULT_CACHE_BLOCK_SIZE / WORD_SIZE;
static int cache_assoc = DEFAULT_CACHE_ASSOC;
static int cache_writeback = DEFAULT_CACHE_WRITEBACK;
static int cache_writealloc = DEFAULT_CACHE_WRITEALLOC;

/* cache model data structures */
static Pcache icache;
static Pcache dcache;
static cache c1;
static cache c2;
static cache_stat cache_stat_inst;
static cache_stat cache_stat_data;

/************************************************************/
void set_cache_param(int param, int value)
{

  switch (param) {
  case CACHE_PARAM_BLOCK_SIZE:
    cache_block_size = value;
    words_per_block = value / WORD_SIZE;
    break;
  case CACHE_PARAM_USIZE:
    cache_split = FALSE;
    cache_usize = value;
    break;
  case CACHE_PARAM_ISIZE:
    cache_split = TRUE;
    cache_isize = value;
    break;
  case CACHE_PARAM_DSIZE:
    cache_split = TRUE;
    cache_dsize = value;
    break;
  case CACHE_PARAM_ASSOC:
    cache_assoc = value;
    break;
  case CACHE_PARAM_WRITEBACK:
    cache_writeback = TRUE;
    break;
  case CACHE_PARAM_WRITETHROUGH:
    cache_writeback = FALSE;
    break;
  case CACHE_PARAM_WRITEALLOC:
    cache_writealloc = TRUE;
    break;
  case CACHE_PARAM_NOWRITEALLOC:
    cache_writealloc = FALSE;
    break;
  default:
    printf("error set_cache_param: bad parameter value\n");
    exit(-1);
  }

}
/************************************************************/

/************************************************************/
void init_cache(void)
{
  /* initialize the cache, and cache statistics data structures */
  /* something */
  int i;
  /* c1 init */
  c1.size = cache_usize;
  c1.associativity = cache_assoc;
  c1.n_sets = cache_usize / (cache_block_size * cache_assoc);
  c1.index_mask = 0x00000ff0;
  c1.index_mask_offset = 4;
  c1.contents = 0;
  if(NULL == (c1.set_contents = (int *)malloc(sizeof(int)*c1.n_sets))){
    printf("c1.set_contents pointer error!!\n");
  }
  for(i=0;i<c1.n_sets;i++){
    c1.set_contents[i] = 0;
  }
  if(NULL == (c1.LRU_head = (Pcache_line *)malloc(sizeof(Pcache_line)*c1.n_sets))){
    printf("c1.LRU_head pointer error!!\n");
  }
  if(NULL == (c1.LRU_tail = (Pcache_line *)malloc(sizeof(Pcache_line)*c1.n_sets))){
    printf("c1.LRU_tail pointer error!!\n");
  }

  /* c1_LRU_head & c1_LRU_tail init */
  for(i=0;i<c1.n_sets;i++){
    c1.LRU_head[i] = (Pcache_line) NULL;
    c1.LRU_tail[i] = (Pcache_line) NULL;
  }

  /* cache_stat_inst init */
  cache_stat_inst.accesses = 0;
  cache_stat_inst.misses = 0;
  cache_stat_inst.replacements = 0;
  cache_stat_inst.demand_fetches = 0;
  cache_stat_inst.copies_back = 0;

  /* cache_stat_data init */
  cache_stat_data.accesses = 0;
  cache_stat_data.misses = 0;
  cache_stat_data.replacements = 0;
  cache_stat_data.demand_fetches = 0;
  cache_stat_data.copies_back = 0;
}
/************************************************************/

/************************************************************/
void perform_access(unsigned addr, unsigned access_type)
{
  /* handle an access to the cache */
  /* something */
  int index;
  unsigned addr_tag;
  Pcache_line temp;
  /* then we start */
  index = ((addr & c1.index_mask) >> c1.index_mask_offset) % c1.n_sets;
  addr_tag = addr & (~c1.index_mask) >> c1.index_mask_offset;
  if(access_type == TRACE_DATA_LOAD){
    cache_stat_data.accesses ++;
    if(NULL != (match_tag(c1.LRU_head[index], c1.LRU_tail[index], addr_tag))){
      //cache_stat_data.hits ++;
    } else if(c1.set_contents[index] == c1.associativity){
      temp = init_Pcache_line(addr_tag);
      delete(&c1.LRU_head[index], &c1.LRU_tail[index], c1.LRU_tail[index]);
      insert(&c1.LRU_head[index], &c1.LRU_tail[index], temp);
      //cache status change
      cache_stat_data.misses++;
      cache_stat_data.demand_fetches += words_per_block;
      cache_stat_data.replacements ++;
    } else{
      temp = init_Pcache_line(addr_tag);
      insert(&c1.LRU_head[index], &c1.LRU_tail[index], temp);
      //cache status change
      c1.set_contents[index]++;
      cache_stat_data.misses ++;
      cache_stat_data.demand_fetches += words_per_block;
      printf("  set_contents[%d] = %d\n",index ,c1.set_contents[index]);
    }
  } else if(access_type == TRACE_DATA_STORE){
    cache_stat_data.accesses ++;
    if(NULL != (temp = match_tag(c1.LRU_head[index], c1.LRU_tail[index], addr_tag))){
      //cache_stat_data.hits ++;
      if(temp->dirty != 1){
        temp->dirty = 1;
      }
    } else if(c1.set_contents[index] == c1.associativity){
      if(c1.LRU_tail[index]->dirty == 1){
        cache_stat_data.copies_back += words_per_block;
      }
      temp = init_Pcache_line(addr_tag);
      delete(&c1.LRU_head[index], &c1.LRU_tail[index], c1.LRU_tail[index]);
      insert(&c1.LRU_head[index], &c1.LRU_tail[index], temp);
      //cache status change
      cache_stat_data.misses ++;
      cache_stat_data.replacements ++;
      cache_stat_data.demand_fetches += words_per_block;
    } else{
      temp = init_Pcache_line(addr_tag);
      temp->dirty = 1;
      insert(&c1.LRU_head[index], &c1.LRU_tail[index], temp);
      //cache status change
      cache_stat_data.misses ++;
      cache_stat_data.demand_fetches += words_per_block;
      c1.set_contents[index]++;
      printf("  set_contents[%d] == %d\n",index ,c1.set_contents[index]);
    }
  } else if(access_type == TRACE_INST_LOAD){
    cache_stat_inst.accesses ++;
    if(NULL != (match_tag(c1.LRU_head[index], c1.LRU_tail[index], addr_tag))){
      //cache_stat_data.hits ++;
    } else if(c1.set_contents[index] == c1.associativity){
      temp = init_Pcache_line(addr_tag);
      delete(&c1.LRU_head[index], &c1.LRU_tail[index], c1.LRU_tail[index]);
      insert(&c1.LRU_head[index], &c1.LRU_tail[index], temp);
      //cache status change
      cache_stat_inst.misses ++;
      cache_stat_inst.demand_fetches += words_per_block;
      cache_stat_inst.replacements ++;
    } else{
      temp = init_Pcache_line(addr_tag);
      insert(&c1.LRU_head[index], &c1.LRU_tail[index], temp);
      //cache status change
      cache_stat_inst.misses ++;
      cache_stat_inst.demand_fetches += words_per_block;
      c1.set_contents[index]++;
      printf("  set_contents[%d] = %d\n",index ,c1.set_contents[index]);
    }
  }
}
/************************************************************/

/************************************************************/
void flush(void)
{
  /* flush the cache */
  /* something */
  int i;
  /* so... */
  for(i=0;i<c1.n_sets;i++){
    if(c1.LRU_head[i] != NULL){
      chain_free(c1.LRU_head[i]);
    }
  }
  free(c1.LRU_head);
  free(c1.LRU_tail);
  free(c1.set_contents);
}
/************************************************************/

/************************************************************/
void delete(Pcache_line* head, Pcache_line* tail, Pcache_line item)
{
  if(item->dirty == 1){
    cache_stat_data.copies_back += words_per_block;
  }
  if (item->LRU_prev) {
    item->LRU_prev->LRU_next = item->LRU_next;
  } else {
    /* item at head */
    *head = item->LRU_next;
  }

  if (item->LRU_next) {
    item->LRU_next->LRU_prev = item->LRU_prev;
  } else {
    /* item at tail */
    *tail = item->LRU_prev;
  }
  free(item);
}
/************************************************************/

/************************************************************/
/* inserts at the head of the list */
void insert(Pcache_line *head, Pcache_line *tail, Pcache_line item)
{
  item->LRU_next = *head;
  item->LRU_prev = (Pcache_line)NULL;

  if (item->LRU_next)
    item->LRU_next->LRU_prev = item;
  else
    *tail = item;
  *head = item;
}
/************************************************************/

/************************************************************/
void dump_settings(void)
{
  printf("*** CACHE SETTINGS ***\n");
  if (cache_split) {
    printf("  Split I- D-cache\n");
    printf("  I-cache size: \t%d\n", cache_isize);
    printf("  D-cache size: \t%d\n", cache_dsize);
  } else {
    printf("  Unified I- D-cache\n");
    printf("  Size: \t%d\n", cache_usize);
  }
  printf("  Associativity: \t%d\n", cache_assoc);
  printf("  Block size: \t%d\n", cache_block_size);
  printf("  Write policy: \t%s\n", 
	 cache_writeback ? "WRITE BACK" : "WRITE THROUGH");
  printf("  Allocation policy: \t%s\n",
	 cache_writealloc ? "WRITE ALLOCATE" : "WRITE NO ALLOCATE");
}
/************************************************************/

/************************************************************/
void print_stats(void)
{
  printf("\n*** CACHE STATISTICS ***\n");

  printf(" INSTRUCTIONS\n");
  printf("  accesses:  %d\n", cache_stat_inst.accesses);
  printf("  misses:    %d\n", cache_stat_inst.misses);
  if (!cache_stat_inst.accesses)
    printf("  miss rate: 0 (0)\n"); 
  else
    printf("  miss rate: %2.4f (hit rate %2.4f)\n", 
	 (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses,
	 1.0 - (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses);
  printf("  replace:   %d\n", cache_stat_inst.replacements);

  printf(" DATA\n");
  printf("  accesses:  %d\n", cache_stat_data.accesses);
  printf("  misses:    %d\n", cache_stat_data.misses);
  if (!cache_stat_data.accesses)
    printf("  miss rate: 0 (0)\n"); 
  else
    printf("  miss rate: %2.4f (hit rate %2.4f)\n", 
	 (float)cache_stat_data.misses / (float)cache_stat_data.accesses,
	 1.0 - (float)cache_stat_data.misses / (float)cache_stat_data.accesses);
  printf("  replace:   %d\n", cache_stat_data.replacements);

  printf(" TRAFFIC (in words)\n");
  printf("  demand fetch:  %d\n", cache_stat_inst.demand_fetches + 
	 cache_stat_data.demand_fetches);
  printf("  copies back:   %d\n", cache_stat_inst.copies_back +
	 cache_stat_data.copies_back);
}
/************************************************************/

/************************************************************/
Pcache_line match_tag(Pcache_line head, Pcache_line tail, unsigned addr_tag){
  if(head != NULL){
    if(head->tag == addr_tag) return head;
    else if(head != tail) return match_tag(head->LRU_next, tail, addr_tag);
    else return NULL;
  } else {
    return NULL;
  }
}
/************************************************************/

/************************************************************/
void chain_free(Pcache_line LRU_chain){
  /*  Let's kill them */
  if(LRU_chain->dirty == 1){
    cache_stat_data.copies_back += words_per_block;
  }
  if(LRU_chain->LRU_next != NULL){
    chain_free(LRU_chain->LRU_next);
  }
  free(LRU_chain);
}
/************************************************************/

/************************************************************/
Pcache_line init_Pcache_line(unsigned addr_tag){
  Pcache_line temp;
  temp = (Pcache_line)malloc(sizeof(cache_line));
  temp->tag = addr_tag;
  temp->dirty = 0;
  temp->LRU_next = (Pcache_line)NULL;
  temp->LRU_prev = (Pcache_line)NULL;
  return temp;
}