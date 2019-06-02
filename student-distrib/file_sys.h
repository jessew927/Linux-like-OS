#ifndef _FILE_SYS_H
#define _FILE_SYS_H

#include "types.h"
#include "task.h"

#define MAX_NAME_LENGTH           32
#define MAX_BLOCK_NUM           1023
#define MAX_FILE_NUM              63
#define BLOCK_SIZE            0x1000
#define BBLOCK_DENTRIES_OFF       64
#define BBLOCK_COUNT_OFF           4
#define INODE_NUM_OFF             36
#define REGULAR_FILE               2
#define BOOT_ENTRIES_OFF           4
#define INODE_ENTRIES_OFF          4

/* data structures are based off of those discussed in lecture 16 */

file_ops_table_t fs_file_ops_table, fs_dir_ops_table;

// File type fields 
typedef int32_t filetype_t;
#define FILE_TYPE_RTC 0
#define FILE_TYPE_DIR 1
#define FILE_TYPE_REG 2

typedef struct dentry{
    int8_t filename[MAX_NAME_LENGTH];
    filetype_t filetype;
    int32_t inode_num;
    uint8_t reserved[24];
} dentry_t;

typedef struct inode{
    uint32_t file_size;
    uint32_t data_blocks[MAX_BLOCK_NUM];
} inode_t;

void fs_init(uint32_t boot_ptr);
int32_t fs_open(const int8_t *filename, FILE *file);

int fs_file_open(const int8_t* filename, FILE *file);
int fs_file_read(int8_t* buf, uint32_t length, FILE *file);
int fs_file_write(const int8_t* buf, uint32_t length, FILE *file);
int fs_file_close(FILE *file);

int fs_dir_open(const int8_t* filename, FILE *file);
int fs_dir_read(int8_t* buf, uint32_t length, FILE *file);
int fs_dir_write(const int8_t* buf, uint32_t length, FILE *file);
int fs_dir_close(FILE *file);

int32_t read_dentry_by_name(const int8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(int32_t inode_num, uint32_t offset, int8_t* buf, uint32_t length);

uint32_t fn_length(const int8_t* fname);

#endif
