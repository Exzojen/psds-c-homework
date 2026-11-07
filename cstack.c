#include "cstack.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define START_CAPACITY 16

struct node
{
    struct node* prev;
    size_t size;
};

struct stack_entry
{
    int reserved;
    unsigned int size;
    struct node* top;
};

struct stack_table
{
    unsigned int capacity;
    unsigned int count;
    struct stack_entry* entries;
};

static struct stack_table g_table = {0, 0, NULL};

static int table_grow(void)
{
    unsigned int new_capacity =
        (g_table.capacity == 0) ? START_CAPACITY : g_table.capacity * 2;
    struct stack_entry* new_entries =
        realloc(g_table.entries, new_capacity * sizeof(struct stack_entry));
    if (new_entries == NULL)
    {
        return 0;
    }
    for (unsigned int i = g_table.capacity; i < new_capacity; ++i)
    {
        new_entries[i].reserved = 0;
        new_entries[i].size = 0;
        new_entries[i].top = NULL;
    }
    g_table.entries = new_entries;
    g_table.capacity = new_capacity;
    return 1;
}

hstack_t stack_new(void)
{
    unsigned int index = 0;
    while ((index < g_table.capacity) && (g_table.entries[index].reserved == 1))
    {
        ++index;
    }
    if ((index == g_table.capacity) && !table_grow())
    {
        return -1;
    }
    g_table.entries[index].reserved = 1;
    g_table.entries[index].size = 0;
    g_table.entries[index].top = NULL;
    ++g_table.count;
    return (hstack_t)index;
}

static void nodes_free(struct node* top)
{
    while (top != NULL)
    {
        struct node* prev = top->prev;
        free(top);
        top = prev;
    }
}

static void table_free(void)
{
    free(g_table.entries);
    g_table.entries = NULL;
    g_table.capacity = 0;
    g_table.count = 0;
}

void stack_free(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return;
    }
    struct stack_entry* entry = &g_table.entries[hstack];
    nodes_free(entry->top);
    entry->top = NULL;
    entry->size = 0;
    entry->reserved = 0;
    --g_table.count;
    if (g_table.count == 0)
    {
        table_free();
    }
}

void stack_free_all(void)
{
    if (g_table.entries == NULL)
    {
        return;
    }
    struct stack_entry* end = g_table.entries + g_table.capacity;
    for (struct stack_entry* entry = g_table.entries; entry != end; ++entry)
    {
        nodes_free(entry->top);
    }
    table_free();
}

int stack_valid_handler(const hstack_t hstack)
{
    return (hstack >= 0) && ((unsigned int)hstack < g_table.capacity) &&
           (g_table.entries[hstack].reserved == 1);
}

unsigned int stack_size(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return 0;
    }
    return g_table.entries[hstack].size;
}

void
stack_push(const hstack_t hstack, const void* data_in, const unsigned int size)
{
    if (!stack_valid_handler(hstack) || (data_in == NULL) || (size == 0))
    {
        return;
    }
    struct node* new_node = malloc(sizeof(struct node) + size);
    if (new_node == NULL)
    {
        return;
    }
    struct stack_entry* entry = &g_table.entries[hstack];
    new_node->prev = entry->top;
    new_node->size = size;
    memcpy(new_node + 1, data_in, size);
    entry->top = new_node;
    ++entry->size;
}

unsigned int
stack_pop(const hstack_t hstack, void* data_out, const unsigned int size)
{
    unsigned int top_size = stack_top_size(hstack);
    if ((top_size == 0) || (data_out == NULL) || (top_size > size))
    {
        return 0;
    }
    struct stack_entry* entry = &g_table.entries[hstack];
    struct node* old_top = entry->top;
    memcpy(data_out, old_top + 1, top_size);
    entry->top = old_top->prev;
    --entry->size;
    free(old_top);
    return top_size;
}

unsigned stack_top_size(const hstack_t hstack)
{
    if (stack_size(hstack) == 0)
    {
        return 0;
    }
    return (unsigned int)g_table.entries[hstack].top->size;
}

hstack_t stack_get_all(void)
{
    hstack_t all = stack_new();
    if (all == -1)
    {
        return -1;
    }
    unsigned int count = 0;
    for (hstack_t i = 0; (unsigned int)i < g_table.capacity; ++i)
    {
        if ((i == all) || !stack_valid_handler(i))
        {
            continue;
        }
        stack_push(all, &i, sizeof(i));
        ++count;
        if (stack_size(all) != count)
        {
            stack_free(all);
            return -1;
        }
    }
    return all;
}
