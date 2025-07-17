#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>



typedef struct Header {
    size_t size;
    bool is_free;
    struct Header *next;
} Header;

static Header *head = NULL;
static Header *tail = NULL;



static void *get_memory_from_os(size_t size) {
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    return mem == MAP_FAILED ? NULL : mem;
}

static void give_memory_back_to_os(void *ptr, size_t len) {
    munmap(ptr, len);
}

static Header *find_free_block(size_t size) {
    Header *current = head;

    while (current != NULL) {
        if (current->is_free && current->size == size)
            return current;
        current = current->next;
    }

    return NULL;
}

static void print_debug(void) {
    Header *current = head;
    while (current != NULL) {
        printf("size: %lu, is_free: %d\n", current->size, current->is_free);
        current = current->next;
    }
}

static void *my_malloc(size_t size) {

    if (size == 0) return NULL;

    Header *current = find_free_block(size);
    if (current != NULL) {
        printf("found free block\n");
        current->is_free = false;
        return (char*) current + sizeof(Header);
    }

    printf("gotta allocate\n");
    Header *hdr = get_memory_from_os(sizeof(Header) + size);
    if (hdr == NULL) return NULL;

    hdr->size = size;
    hdr->is_free = false;
    hdr->next = NULL;

    if (head == NULL)
        head = hdr;
    else
        tail->next = hdr;

    tail = hdr;

    return (char*) hdr + sizeof(Header);

}

static void *my_calloc(size_t n, size_t size) {
    void *mem = my_malloc(n*size);
    if (n == 0 || size == 0) return NULL;
    memset(mem, 0, n*size);
    return mem;
}

static void my_free(void *ptr) {
    if (ptr == NULL) return;

    Header *current = head;
    while (current != NULL) {
        if ((char*) current + sizeof(Header) == ptr) {
            current->is_free = true;
            return;
        }
        current = current->next;
    }

    assert(!"block not found");

}

static void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL) return my_malloc(size);

    Header *hdr = (Header*) ((char*) ptr - sizeof(Header));
    if (size <= hdr->size) return ptr;

    void *mem = my_malloc(size);
    if (mem == NULL) return NULL;

    memcpy(mem, ptr, hdr->size);
    my_free(ptr);

    return mem;
}

typedef char Item;

typedef struct {
    Item *items;
    size_t cap, len;
} Dynarray;

static void dynarray_init(Dynarray *dyn) {
    dyn->cap = 5;
    dyn->len = 0;
    dyn->items = my_malloc(dyn->cap * sizeof(Item));
    assert(dyn->items != NULL);
}

static void dynarray_append(Dynarray *dyn, Item item) {

    if (dyn->len >= dyn->cap) {
        dyn->cap *= 2;
        dyn->items = my_realloc(dyn->items, dyn->cap);
        assert(dyn->items != NULL);
    }

    dyn->items[dyn->len++] = item;
}

static void dynarray_destroy(Dynarray *dyn) {
    my_free(dyn->items);
}

int main(void) {

    Dynarray dyn = { 0 };
    dynarray_init(&dyn);
    for (size_t i=0; i < 50; ++i)
        dynarray_append(&dyn, i+1);
    dynarray_destroy(&dyn);

    int *x = my_malloc(sizeof(int));
    *x = 45;
    assert(*x == 45);

    int *y = my_malloc(sizeof(int));
    *y = 45;
    assert(*y == 45);

    int *z = my_malloc(sizeof(int));
    *z = 45;
    assert(*z == 45);

    int *array = my_malloc(5*sizeof(int));
    for (size_t i=0; i < 5; ++i) {
        array[i] = i+1;
    }

    my_free(y);

    my_malloc(sizeof(int));

    print_debug();

    return EXIT_SUCCESS;
}
