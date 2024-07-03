// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
// defined by kernel.ld.

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
    int refcount[(PHYSTOP >> PGSHIFT)]; // Array to keep track of reference counts
    int total_pages; // Total number of pages
    int free_pages;  // Number of free pages
} kmem;


int
total_memory_size()
{
    int x_total_memory_size;
    acquire(&kmem.lock);

    x_total_memory_size = kmem.total_pages * PGSIZE;

    release(&kmem.lock);

    return x_total_memory_size;
}

int
free_memory_size()
{
    int x_free_memory_size;
    acquire(&kmem.lock);

    x_free_memory_size = kmem.free_pages * PGSIZE;

    release(&kmem.lock);

    return x_free_memory_size;
}

void
kinit()
{
    initlock(&kmem.lock, "kmem");
    kmem.total_pages = 0;
    kmem.free_pages = 0;
    freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
        kfree(p);
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

    acquire(&kmem.lock);

    uint64 pa_index = ((uint64)pa) >> PGSHIFT;
    if(kmem.refcount[pa_index] > 1) {
        kmem.refcount[pa_index]--;
    } else {
        // Fill with junk to catch dangling refs.
        memset(pa, 1, PGSIZE);
        kmem.refcount[pa_index] = 0;

        kmem.free_pages++; // Increment free_pages

        if (kmem.free_pages > kmem.total_pages)
            kmem.total_pages = kmem.free_pages;

        r = (struct run*)pa;
        r->next = kmem.freelist;
        kmem.freelist = r;
    }

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
    if(r) {
        kmem.free_pages--; // Decrement free_pages
        kmem.freelist = r->next;
        uint64 pa_index = ((uint64)r) >> PGSHIFT;
        kmem.refcount[pa_index] = 1; // Initialize refcount to 1 for newly allocated page
    }
    release(&kmem.lock);

    if(r)
        memset((char*)r, 5, PGSIZE); // fill with junk
    return (void*)r;
}

// Increment the reference count of the physical page at pa
void increment_refcount(void *pa) {
    acquire(&kmem.lock);
    uint64 pa_index = ((uint64)pa) >> PGSHIFT;
    kmem.refcount[pa_index]++;
    release(&kmem.lock);
}

// Decrement the reference count of the physical page at pa
void decrement_refcount(void *pa) {
    acquire(&kmem.lock);
    uint64 pa_index = ((uint64)pa) >> PGSHIFT;
    if (kmem.refcount[pa_index] > 0) {
        kmem.refcount[pa_index]--;
        if (kmem.refcount[pa_index] == 0) {
            kfree(pa);
        }
    }
    release(&kmem.lock);
}
