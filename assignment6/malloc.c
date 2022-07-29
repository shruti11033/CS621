#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "malloc2.h"

int *addrI; //Starting address
int lenI; //length from size parameter
/*Definition of data structure to keep memory allocation information. To be used for memcheck2 function */
typedef
struct _mem_Node
{
void *address; // Address of allocated memory
size_t nbytes; // Number of bytes allocated
} mem_node;

/*
malloc2 allocates memory using malloc funtion and record tuple for memory allocated in heap and length and prform validation checks if operation is safe.

*/
int *malloc2(int size){
if(size <= 0){
printf("Memory size should be greate then zero\n");
exit(0);
}

addrI = (int*)malloc(sizeof(int));
if(addrI == NULL){
printf("Memory allocation unsuccessful\n");
exit(0);
}
return addrI;
}

/*
Free2 function first checks if operation is safe and then
frees the allocated memory using call to free();
*/
int free2(void *ptr){
if (ptr != NULL){
free(ptr);
ptr = NULL;
//exit(0);
}else if (ptr != addrI){
printf("Error: Not the first byte of the range of memory that was allocated\n");
exit(0);
}else if(ptr == NULL){
printf("Error: Freeing memory that was previously freed.\n");
}else{
printf("Error:Freeing memory not allocated by malloc2\n");
exit(0);
}
return 0;
}

/*
Realloc2 function changes memory alloction by calling realloc
after performing initial checks for valid allocation.
*/
int *realloc2(void *ptr, int size){
if(ptr == NULL){
if(size <= 0){
return 0;
}else{
return malloc2(size);
}
}else{
if(size == 0){
int i= free2(ptr);
return 0;
}
}

void *ptr2;
ptr2 = realloc(ptr,size);
if(ptr2 == NULL){
printf("Realloc value is null\n");
exit(-1);
}
memcpy(addrI, ptr,size);
free(ptr);
return 0;

}
/*
Function checks address range specified by address pointer
length size allocated.
*/

int memcheck2(void *addr, int size){

if(addr == NULL){
printf("Error: Null pointer specified\n");
exit(-1);
}
if(size == 0){
printf("Error: Memorys size zero\n");
}
void *ptr2,*ptr_new;
ptr2 = &addr + size;
if(ptr2 == NULL){
printf("Error: Address not allocated");
// exit(-1);
}

ptr_new = (int *)malloc2(sizeof(size));

if(ptr2 == ptr_new){
printf("Memory check failed\n");
exit(-1);
}
return 0;
}

/*
main function contains test cases to check above code of
malloc2, free2 and memcheck2. It can be created in seprate file
and included seprately.
*/

int main(int argc,char* argv[]){
int *ptr;
int size =16;
ptr = (int *)malloc2(sizeof (size));
if(ptr == NULL)
{ printf("error\n");
return 1;
}
else
{ printf(" Malloc2 successful\n");
}
memcheck2(ptr, 36);
printf("Memcheck successfull\n");
free2(ptr);
printf("free Successfull\n");

char buff = (char)malloc(10);
memcheck2(buff,strlen("hi hello!"));
strcpy(buff,"hi hello!");
return 0;

}
