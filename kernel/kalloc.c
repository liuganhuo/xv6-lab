// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

struct {
    int ref[PHYSTOP >> PGSHIFT];
    struct spinlock cow_lock;
} cows;

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  struct run *r;

  p = (char*)PGROUNDUP((uint64)pa_start);
  for (int i = 0; i < (uint64)p/PGSIZE; i++)
    cows.ref[(uint64)p/PGSIZE] = 0;
  
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    memset(p, 1, PGSIZE);

    r = (struct run *)p;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  acquire(&cows.cow_lock);
  if (--cows.ref[(uint64)pa >> PGSHIFT] != 0){
    release(&cows.cow_lock);
    return;
  }
  release(&cows.cow_lock);
  
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&cows.cow_lock);
    cows.ref[(uint64)r/PGSIZE] = 1;
    release(&cows.cow_lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
inc_refcount(uint64 pa) {
  acquire(&cows.cow_lock);
  cows.ref[pa >> PGSHIFT]++;
  release(&cows.cow_lock);
}

/* Do the following steps atomically:
 *  1. if page's reference count is 1(i.e., not shared), return true. 
 *  2. else, decrease page's reference count, return false.
 */
int
dec_refcount_if_shared(uint64 pa) {
  acquire(&cows.cow_lock);
  uint8 ret = cows.ref[pa >> PGSHIFT];
  if (ret != 1) {
    --cows.ref[pa >> PGSHIFT];
    ret = 0;
  } else {
    ret = 1;
  }
  release(&cows.cow_lock);
  return ret;
}