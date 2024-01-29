#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uthash.h"

#define FLAG_IMPLEMENTATION
#include "flag.h"

void usage(FILE *sink, const char *program)
{
    fprintf(sink, "Usage: %s [OPTIONS] [--] <blacklist-file> ... \n\n", program);
    fprintf(sink, "    blacklist-file: reads stdin and only if the line is not in the blacklist-file, it is printed to stdout\n\n");
    fprintf(sink, "OPTIONS:\n");
    flag_print_options(sink);
}

// Function to read the contents of a file into memory
char* readFile(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Allocate memory to store the file content
    char *buffer = (char *)malloc(fileSize + 1);
    if (!buffer) {
        perror("Error allocating memory");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Read the file into the buffer
    *size = fread(buffer, 1, fileSize, file);
    if (*size != fileSize) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Null-terminate the buffer
    buffer[fileSize] = '\0';

    // Close the file
    fclose(file);

    return buffer;
}

// Define a for storing key-value pairs
typedef struct {
    char *key;
    UT_hash_handle hh;
} KeyHash;

typedef struct {
    KeyHash *hashTable;
    KeyHash *entries;
} FileHash;

_Bool isLineSep(char ch) {
    return ch == '\n' || ch == '\0';
}

// Function to read all lines from a file and insert into the hash table
void hashFile(char *filebuff, size_t buffsize, FileHash *fileHash) {
    size_t line_count = 0;

    char *start = filebuff; 
    char *ch = filebuff;
    for (int j=0; j < buffsize; ++j, ++ch) {
        if (isLineSep(*ch)) {
            if (ch - start != 0) // Only count non-empty lines
                ++line_count;
            start = ch + 1;
        }
    }
    if (!isLineSep(*ch)) {  // If the file does not end in a newline
        if (ch - start != 0)
            ++line_count;
    }
   
    KeyHash *entries = malloc(line_count * sizeof(KeyHash));
    fileHash->entries = entries;
    
    KeyHash *hashTable = NULL;
    
    KeyHash *entry;
    size_t counter = 0;
    start = ch = filebuff;
    for (int j=0; j < buffsize; ++j, ++ch) {
        if (!isLineSep(*ch))
            continue;

        if (ch - start != 0) { // Skip empty lines
            entries[counter].key = start;
            *ch = '\0'; // TODO: Check for uniqueness

            HASH_FIND(hh, hashTable, start, ch-start, entry);
            if (!entry) {
                HASH_ADD_KEYPTR(hh, hashTable, start, ch-start, &entries[counter]);
                ++counter;
            }
        }
        start = ch + 1;
    } 
    // If the file does not end in a newline
    if (!isLineSep(*ch) && ch - start != 0) { 
        entries[counter].key = start;
        HASH_FIND(hh, hashTable, start, ch-start, entry);
        if (!entry) {
            HASH_ADD_KEYPTR(hh, hashTable, start, ch-start, &entries[counter]);
            ++counter;
        }
    }

    fileHash->hashTable = hashTable;
}
 
// Function to print the contents of the hash table
void printHashTable(const KeyHash *hashTable) {
    const KeyHash *entry;
    for (entry = hashTable; entry != NULL; entry = entry->hh.next) {
        printf("Key: %s\n", entry->key);
    }
}

// Function to free the memory used by the hash table
void freeFileHash(FileHash *fileHash) {
    KeyHash *current, *tmp;

    HASH_ITER(hh, fileHash->hashTable, current, tmp) {
        HASH_DEL(fileHash->hashTable, current);
    }
    free(fileHash->entries);
}

#define MAX_LINE_LENGTH 1024
#define UNIQUE  1
#define WHITELIST  2

void 
parse(FILE *sink, FILE *source, FileHash *blacklist, uint32_t flags)
{
    char buffer[MAX_LINE_LENGTH];

    KeyHash *hashTable = NULL;
    KeyHash *entry;

    char *line = NULL;
    size_t len = 0;

    // Read lines from stdin until EOF is encountered
    while (getline(&line, &len, stdin) != -1) {
        // Remove newline character
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
            line_len--;
        }

        HASH_FIND(hh, blacklist->hashTable, line, line_len, entry);
        if (( !(flags & WHITELIST) && entry) ||
            ( (flags & WHITELIST) && !entry) ) // Skip line if in blacklist
            continue;

        HASH_FIND(hh, hashTable, line, line_len, entry);
        if (entry) { // Line already printed
            if (flags & UNIQUE)
                continue;
        } 
        else {
            entry = malloc(sizeof(KeyHash));
            entry->key = line;

            HASH_ADD_KEYPTR(hh, hashTable, entry->key, line_len, entry);
        }

        fprintf(sink, "%s\n", line);
    }


    KeyHash *current, *tmp;
    HASH_ITER(hh, hashTable, current, tmp) {
        HASH_DEL(hashTable, current);
        free(current);
    }

    free(line);
}

int main(int argc, char *argv[]) {

    const char *program = *argv;

    bool *help = flag_bool("help", 'h', "Print this help to stdout and exit with 0");
    bool *uniq = flag_bool("uniq", 'u', "Print a line only the first time it occurs");
    bool *whitelist = flag_bool("whitelist", 'w', "Argument file is a whilelist in stead of a blacklist");
    
    if (!flag_parse(argc, argv)) {
        usage(stderr, program);
        flag_print_error(stderr);
        exit(1);
    }
    argv = flag_rest_argv();
    
    if (*help) {
        usage(stdout, program);
        exit(0);
    }

    /* if (!argv && !uniq) {
        fprintf(stderr, "Usage: %s <filename>\n", program);
        return EXIT_FAILURE;
    } */ 

    FileHash blacklist = { 0 };
    if (argv[0]) {
        const char *filename = argv[0];

        size_t file_size;
        char *filebuff = readFile(filename, &file_size);
        hashFile(filebuff, file_size, &blacklist);
    }

    uint32_t flags = *uniq * UNIQUE + *whitelist * WHITELIST;
    parse(stdout, stdin, &blacklist, flags);

    freeFileHash(&blacklist);
    return EXIT_SUCCESS;
}
