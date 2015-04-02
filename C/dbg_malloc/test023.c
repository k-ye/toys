#include "m61.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
// More boundary write error checks #1.

int main() {
    const char* string1 = "Hello, this is a string! I exist to demonstrate a common error.";
    char* copy_of_string1 = (char*) malloc(strlen(string1));
        // Whoops! Forgot to allocate space for the '\0' that ends the string.
    //printf("alloc len: %lu\n", strlen(string1));
    strcpy(copy_of_string1, string1); // == boundary write error
    //printf("actual str len: %lu\n", strlen(copy_of_string1));
    //printf("last %c\n", copy_of_string1[62]);
    free(copy_of_string1);
    m61_printstatistics();
}

//! MEMORY BUG???: detected wild write during free of pointer ???
//! ???
