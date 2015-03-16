#include<stdio.h>

#define MAX_MEM 1024;

class free_alloc
{
public:
    free_alloc* next;
    int begin_addr, size;

    free_alloc(int begin_addr, int size, free_alloc* next)
        :begin_addr(begin_addr), size(size), next(next){}
};
typedef free_alloc used_alloc;

class pmm_manager
{
    //first-fit allocation.
public:
    pmm_manager(int size, int addr);
    int malloc_pmm(int size);
    void free_pmm(int addr);
private:
    int init_addr;
    free_alloc* freea;
    used_alloc* useda;
};

pmm_manager::pmm_manager(int size, int addr)
{
    init_addr = addr;
    freea = new free_alloc(addr, size, NULL);
    useda = NULL;
    printf("Init pmm_manager at %d with %d size.\n", addr, size);
}

int pmm_manager::malloc_pmm(int size)
/*
 *  return with used memory's begin address.
 *  -1 for no result.
 */
{
    printf("Start malloc_pmm\n");
    free_alloc* p = freea, *q = NULL, *temp;
    int result = -1;
    while(p)
    {
        if (p->size >= size)
        {
            result = p->begin_addr;
            temp = new free_alloc(p->begin_addr + size, p->size - size, p->next);
            delete p;
            p = temp;

            printf("Find free space at %d with %d size.\n", result, size);
            temp = new free_alloc(result, size, useda);
            useda = temp;
            printf("After change useda.\n");
            if (q)
                q->next = p;
            else
                freea = p;
            return result;
        }

        q = p;
        p = p->next;
    }
    printf("No free size!\n");
}

void pmm_manager::free_pmm(int addr)
{
    used_alloc* p = useda, *q = NULL;
    while(p)
    {
        if (p->begin_addr == addr)
        {
            if (q)
                q->next = p->next;
            else
                useda = p->next;

            free_alloc *temp = freea, *temp2 = NULL;
            while(temp)
            {
                if (temp->begin_addr == p->begin_addr + p->size)
                {
                    printf("Merge with block at %d with %d size.\n", temp->begin_addr, temp->size);
                    temp->begin_addr = p->begin_addr;
                    temp->size += p->size;
                    delete p;
                    break;
                }
                if (temp->begin_addr + temp->size == p->begin_addr)
                {
                    printf("Merge with block at %d with %d size.\n", temp->begin_addr, temp->size);
                    temp->size += p->size;
                    delete p;
                    break;
                }
                if (temp->begin_addr > p->begin_addr)
                {
                    printf("Cannot merge with other free memory, Add into free blocks.\n");
                    p->next = temp;
                    if (temp2)
                        temp2->next = p;
                    else
                        freea = p;
                }
                temp2 = temp;
                temp = temp->next;
            }
            break;
        }
        q = p;
        p = p->next;
    }
}

int main()
{
    printf("Start\n");
    pmm_manager pm(0x500, 0x00);
    int a1 = pm.malloc_pmm(0x100);
    int a2 = pm.malloc_pmm(0x50);
    int a3 = pm.malloc_pmm(0x30);
    int a4 = pm.malloc_pmm(0x40);
    printf("Four blocks at %d, %d, %d, %d.\n", a1, a2, a3, a4);
    pm.free_pmm(a2);
    pm.free_pmm(a3);
    return 0;
}
