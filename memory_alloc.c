#include<stdio.h>
#include<unistd.h>
#include<string.h>

typedef struct mem_block
{
    int free;
    struct mem_block *next_block;
    size_t memory;

} mem_block;

#define BLOCK_SIZE sizeof(mem_block); //metadata size

mem_block * head  = NULL ; //our memory pool 

int init_pool(size_t amount)
{
   mem_block *memory_block = NULL; 
   memory_block = sbrk(amount);
   if(memory_block == (void *)-1){
        printf("failed sbrk ()");
        return -1;
   }

   memory_block ->free = 1;
   memory_block ->memory = amount - sizeof(mem_block);
   memory_block ->next_block = NULL;

   if(head == NULL)
        head = memory_block ; 
    else {
        mem_block *current = head;
        while(current->next_block)
            current = current->next_block;
        current->next_block = memory_block ;

      //  printf("added block : %p \n", memory_block);
        //printf("new memory of memory_block : %zu \n" , memory_block->memory);
        //printf("Plus block : %p \n ", current->next_block);
    }
    return 0;

}
void *mem_alloc(size_t memory_size)
{
    if(memory_size <= 0){
        perror("Impossible");
        return NULL;
    }
    if(head == NULL){
        if(init_pool(1024*1024) == -1){
            printf("Error failed, problms with init_pool() inside the mem_alloc func");
            return NULL;
        } 
    }
    //edge case of memory_size requested by user to be bigger than the initial pool 
   
    //calculating total size ( making the size requested to be multiple of 8)
    size_t total_mem = memory_size + sizeof(mem_block);

    if(total_mem % 8 != 0)
        total_mem += (8 - (total_mem % 8));

     if(total_mem > head->memory) // if the allocation size is bigger than the pool we dynamically change the pool
    {
       // printf("Pool size before : %zu\n" , head->memory); 
        size_t new_size = total_mem + sizeof(mem_block) ;
        if(init_pool(new_size) == -1){
            printf("Erros with bigger init_pool ()");
            return NULL;
        }
    }
   // printf("total size : %d \n", total_mem);
    mem_block *current_block;
    current_block = head; //first block starts from the pool
    if(current_block == NULL)
        printf("FUCK ME");

    while(current_block) //searching for memory in the pool
    {
       
      //  printf("Current_block ->free : %d \n" ,current_block->free); 
       // printf("Current_block->memory :%zu \n" , current_block->memory); 
        //printf("memory size  : %zu \n" , total_mem);
        if(current_block ->free == 1 && current_block ->memory >= total_mem)
        {
            //printf("Got into if -----------") ;
            if(current_block->memory >= total_mem + sizeof(mem_block) + 8){
                mem_block * split_block = (mem_block *)((char *)current_block + total_mem + sizeof(mem_block)); // adding the size of memblock thats the metadata 
                split_block->free = 1; //we are moving the memory from the pool to the new split_block , we will use current_block's memory 
                split_block->memory = current_block->memory - total_mem - sizeof(mem_block);
                split_block->next_block = current_block->next_block; 

                current_block->memory = total_mem;
                current_block->next_block = split_block;
            } 

            current_block->free = 0;
            return (void *) current_block + sizeof(mem_block);

        }

        current_block = current_block ->next_block;
    }
    
    //case no block found we just make a new one;
    if(init_pool(total_mem) == -1){
        printf("Allocation failed line 101") ;
        return NULL;
    }
}

int pointer_validation(void * pointer_to_val)
{
    if(pointer_to_val == NULL)
        return 0;

    int valid = 0;
    mem_block *pointer_check = head; 
    while(pointer_check && !valid)
    {
        void *user_data = (void *)(pointer_check + 1); //moving past the metadata of the pointer 
        if(user_data == pointer_to_val)
            valid = 1;  
        pointer_check = pointer_check->next_block;
    }
    return valid;
}

void mem_free(void *pointer_to_a_block)
{
    if(pointer_to_a_block == NULL)
        return; 

    //pointer validation ( to check if the freed pointer is from the memory pool not outside )
    if (!pointer_validation(pointer_to_a_block)) 
    {
        perror("Not a valid pointer");
        return;
    }
    mem_block * block = (mem_block *)pointer_to_a_block -1; //returning the memory to the previous block
    block->free = 1;
    
    mem_block * current  = head; //starting from the head  to remove the fragmentation issue 
    while(current->next_block)
    {
        if(current->free == 1 && current->next_block->free == 1 )
        {
            current->memory += current->next_block->memory;
            current->next_block = current->next_block->next_block;   
        }
        else 
            current = current->next_block;

    } 
}

int main() {
    // Test 1: Initialization of Memory Pool
    printf("=== Test Initialization ===\n");
    void *ptr1 = mem_alloc(128);
    if (ptr1) {
        printf("Memory allocated successfully at %p\n", ptr1);
        printf("Test Initialization: SUCCESS\n");
    } else {
        printf("Test Initialization: FAILURE\n");
    }
    printf("=== End Test Initialization ===\n\n");

    // Test 2: Basic Memory Allocation
    printf("=== Test Basic Memory Allocation ===\n");
    ptr1 = mem_alloc(128);
    if (ptr1) {
        strcpy((char *)ptr1, "Hello, World!");
        printf("Data in allocated memory: %s\n", (char *)ptr1);
        printf("Test Basic Memory Allocation: SUCCESS\n");
    } else {
        printf("Test Basic Memory Allocation: FAILURE\n");
    }
    mem_free(ptr1); // Free the allocated memory
    printf("=== End Test Basic Memory Allocation ===\n\n");

    // Test 3: Allocation Beyond Initial Pool Size
    printf("=== Test Allocation Beyond Initial Pool Size ===\n");
    void *ptr2 = mem_alloc(2 * 1024 * 1024); // 2 MB
    if ( ptr2 ) {
        printf("2 MB memory allocated successfully at %p\n", ptr2);
        printf("Test Allocation Beyond Initial Pool Size: SUCCESS\n");
    } else {
        printf("Test Allocation Beyond Initial Pool Size: FAILURE\n");
    }
    mem_free(ptr2); // Free the allocated memory
    printf("=== End Test Allocation Beyond Initial Pool Size ===\n\n");
    
    // Test 4: Multiple Allocations
    printf("=== Test Multiple Allocations ===\n");
    void *ptr3 = mem_alloc(256);
    if (ptr3) {
        printf("256 bytes memory allocated at %p\n", ptr3);
        mem_free(ptr3); // Free the allocated memory
        printf("Test Multiple Allocations: SUCCESS\n");
    } else {
        printf("Test Multiple Allocations: FAILURE\n");
    }
    printf("=== End Test Multiple Allocations ===\n\n");

    // Test 5: Memory Freeing
    printf("=== Test Memory Freeing ===\n");
    ptr1 = mem_alloc(128);
    mem_free(ptr1); // Free the allocated memory
    printf("Memory freed successfully.\n");
    ptr1 = mem_alloc(128); // Reallocate to check reuse
    if (ptr1) {
        printf("Reallocation successful at %p\n", ptr1);
        printf("Test Memory Freeing: SUCCESS\n");
    } else {
        printf("Test Memory Freeing: FAILURE\n");
    }
    mem_free(ptr1); // Free again
    printf("=== End Test Memory Freeing ===\n\n");

    // Test 6: Double Free Prevention
    printf("=== Test Double Free Prevention ===\n");
    ptr1 = mem_alloc(128);
    mem_free(ptr1); // First free
    printf("First free successful.\n");
    mem_free(ptr1); // Attempt to double free
    printf("Second free attempted.\n");
    printf("Test Double Free Prevention: SUCCESS (if no crash)\n");
    printf("=== End Test Double Free Prevention ===\n\n");

    // Test 7: Pointer Validation
    printf("=== Test Pointer Validation ===\n");
    ptr1 = mem_alloc(128);
    if (ptr1) {
        if (pointer_validation(ptr1)) {
            printf("Pointer validation successful for %p\n", ptr1);
            printf("Test Pointer Validation: SUCCESS\n");
        } else {
            printf("Test Pointer Validation: FAILURE\n");
        }
    }
    mem_free(ptr1); // Free the allocated memory
    printf("=== End Test Pointer Validation ===\n\n");
    return 0;
}
