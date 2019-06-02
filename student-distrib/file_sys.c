#include "file_sys.h"
#include "lib.h"

// File ops table
file_ops_table_t fs_file_ops_table = {
    .open = fs_file_open,
    .read = fs_file_read,
    .write = fs_file_write,
    .close = fs_file_close,
};

file_ops_table_t fs_dir_ops_table = {
    .open = fs_dir_open,
    .read = fs_dir_read,
    .write = fs_dir_write,
    .close = fs_dir_close,
};

/* Global Variables */
static uint32_t bblock_ptr;
static inode_t* inodes;
static dentry_t* dentries;

/* Function: fs_init;
 * Inputs: boot_ptr - the ptr the boot block
 * Return Value: None
 * Function: Initializes global variables
 */
void fs_init(uint32_t boot_ptr){
    bblock_ptr = boot_ptr;
    dentries = (dentry_t*)(bblock_ptr + BBLOCK_DENTRIES_OFF);
    inodes = (inode_t*)(bblock_ptr + BLOCK_SIZE);
}


int32_t fs_open(const int8_t *filename, FILE *file) {
    dentry_t dent;
    if( read_dentry_by_name(filename, &dent) != 0 ){
        return -1;
    }

    file->pos = 0;
    file->inode = dent.inode_num;
    if (dent.filetype == FILE_TYPE_REG) {
        file->file_ops = &fs_file_ops_table;
        file->flags.type = TASK_FILE_REG;
    } else {
        file->file_ops = &fs_dir_ops_table;
        file->flags.type = TASK_FILE_DIR;
    }
    return 0;
}

// ======== File operation functions ========
/* Function: fs_file open;
 * Inputs: filename - the file name we want to open
 * Return Value: 0
 * Function: Initializes the cur_file global variable to hold the current file data we want to read
 */
int fs_file_open(const int8_t* filename, FILE *file){
    return 0;
}

/* Function: fs_file_read;
 * Inputs: buf - the buffer we want to copy the file data to
 *         length - the number of bytes we want to read
 * Return Value: The number of bytes read
 * Function: Reads the current file
 */
int fs_file_read(int8_t* buf, uint32_t length, FILE *file){
    int bytes_read = (int)read_data(file->inode, file->pos, buf, length);
    file->pos += bytes_read;
    return bytes_read;
}

/* Function: fs_file_write;
 * Inputs: buf - the buffer we want to write the file data to
 *         length - the number of bytes we want to write
 * Return Value: -1
 * Function: Nothing
 */
int fs_file_write(const int8_t* buf, uint32_t length, FILE *file){ return -1; }

/* Function: fs_file_close;
 * Inputs: filename - the file name we want to close
 * Return Value: 0
 * Function: Resets the cur_file variable
 */
int fs_file_close(FILE *file){
    return 0;
}



// ======== Directory operation functions ========
/* Function: fs_dir_open;
 * Inputs: filename - the file name we want to open
 * Return Value: 0
 * Function: Initializes the cur_index global variable to 0
 */
int fs_dir_open(const int8_t* filename, FILE *file){
    return 0;
}

/* Function: fs_dir_read;
 * Inputs: buf - the buffer we want to copy the file name to
 * Return Value: the length of the file name
 * Function: Reads the current directory entry
 */
int fs_dir_read(int8_t* buf, uint32_t length, FILE *file){
    dentry_t read_dentry;
    if (read_dentry_by_index(file->pos, &read_dentry)) {
        return 0;
    }
    strncpy((int8_t*)buf, (int8_t*)read_dentry.filename, MAX_NAME_LENGTH);
    file->pos++;
    return fn_length(buf);
}

/* Function: fs_dir_write;
 * Inputs: buf - the buffer we want to write the file data to
 *         length - the number of bytes we want to write
 * Return Value: -1
 * Function: Nothing
 */
int fs_dir_write(const int8_t* buf, uint32_t length, FILE *file){ return -1; }

/* Function: fs_dir_open;
 * Inputs: filename - the file name we want to open
 * Return Value: 0
 * Function: Resets the cur_index global variable to 0
 */
int fs_dir_close(FILE *file){
    return 0;
}



/* Function: read_dentry_by_name
 * Inputs: fname - the name of the file
 *         dentry - the dentry variable we want to copy data to
 * Return Value: -1 if failed, 0 if success
 * Function: Copies the data to the dentry variable
 */
int32_t read_dentry_by_name(const int8_t* fname, dentry_t* dentry){
    /* loop index */
    int i;
    /* temp name holder */
    /* int8_t cur_name[MAX_NAME_LENGTH]; */
    int32_t num_dentries = (int32_t)(*((uint32_t*)bblock_ptr));
    uint32_t name_length;
    if (!(name_length = fn_length(fname))) {
        return -1;
    }

    /* loops through the dentries, skip the first one because that refers to the
      directory itself
    */
    for(i = 0; i < num_dentries; i++){
        /* set cur_name to the current dentry filename */
        /* strncpy(cur_name, (int8_t*)dentries[i].filename, name_length); */
        /* check to see if the names are the same
          if they are then copy the relevant values to dentry */
        if(!strncmp((int8_t*)dentries[i].filename, (int8_t*)fname, MAX_NAME_LENGTH)){
          (void)read_dentry_by_index(i, dentry);
          return 0;
        }
    }

    return -1;
}

/* Function: read_dentry_by_index
 * Inputs: index - the index of the file in the directory
 *         dentry - the dentry variable we want to copy data to
 * Return Value: -1 if failed, 0 if success
 * Function: Copies the data to the dentry variable
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    /*local vars to hold the data we want to copy */
    int32_t file_type;
    int32_t inode_num;
    uint32_t num_dentries = *(uint32_t*)(bblock_ptr);
    /* check if the index is within acceptable bounds */
    if(index >= num_dentries || index < 0)
        return -1;
    /* get the relevant data to copy */
    file_type = (int32_t)(dentries[index].filetype);
    inode_num = (int32_t)(dentries[index].inode_num);

    /* set dentry to be a copy of the corresponding file sys dentry */
    dentry->filetype = file_type;
    dentry->inode_num = inode_num;
    /* Names do not necessarily include a terminal EOS */
    strncpy((int8_t*)dentry->filename, (int8_t*)dentries[index].filename, MAX_NAME_LENGTH);

    return 0;
}

/* Function: read_data
 * Inputs: inode_num - the file's inode number
 *         offset - the number of bytes already read
 *         buf - the buffer we want to copy data to
 *         length - the number of bytes we want to read
 * Return Value: The number of bytes read
 * Function: Copies the data to the buffer
 */
int32_t read_data(int32_t inode_num, uint32_t offset, int8_t* buf, uint32_t length){
    /* for loop index */
    int i;
    /* local variables used to find byte data*/
    int32_t inode_count;
    int32_t data_block_count;
    uint32_t cur_data_block;
    int32_t data_offset;
    uint32_t cur_byte;
    int starting_index;
    int32_t actual_len;
    int8_t* cur_buf_pos;
    int32_t num_bytes = 0;

    /* get the number of inodes */
    inode_count = (int32_t)(*((uint32_t*)(bblock_ptr + BBLOCK_COUNT_OFF)));

    /* checks to see if the inode_num is valid */
    if(inode_num > inode_count || inode_num < 0)
      return 0;

    /* get the number of data_blocks */
    data_block_count = (int32_t)(*((uint32_t*)(bblock_ptr + 2 * BOOT_ENTRIES_OFF)));

    /* checks to see if the entire file has already been read, return 0 if it has */
    if(offset >= inodes[inode_num].file_size)
      return 0;

    /* calculates the actual length to be read, truncates the length if it goes over the
      amount of bytes that have yet to be read */
    if( (length + offset > inodes[inode_num].file_size) )
      actual_len = inodes[inode_num].file_size - offset;
    else
      actual_len = length;

    /* store buf address with a temp variable that will be iterated through */
    cur_buf_pos = buf;

    /* calculates the index of which data block number to start on */
    starting_index = offset / BLOCK_SIZE;
    /* get the starting data block */
    cur_data_block = inodes[inode_num].data_blocks[starting_index];
    /* calculates where in the file to start reading from */
    data_offset = offset % BLOCK_SIZE;

    /* get the address of the first byte we want to read */
    cur_byte = bblock_ptr + inode_count * BLOCK_SIZE + cur_data_block * BLOCK_SIZE + data_offset + BLOCK_SIZE;

    /* loops through all the bytes to be read and copying them to the buffer */
    for(i = 0; i < actual_len; i++){
        /* checks to see if we should switch to another data block */
        if( !((i + data_offset) % BLOCK_SIZE) && (i + data_offset) ){
          starting_index++;
          cur_data_block = inodes[inode_num].data_blocks[starting_index];
          cur_byte = bblock_ptr + inode_count * BLOCK_SIZE + (cur_data_block) * BLOCK_SIZE + BLOCK_SIZE;
        }
        /* copy byte data to buffer */
        *cur_buf_pos = (int8_t)(*((uint32_t*)cur_byte));
        /* update the buffer to next position to fill */
        cur_buf_pos++;
        /* update byte address to the next byte to be read */
        cur_byte++;
        /* increment the number of bytes read counter */
        num_bytes++;
    }

    /* return the number of bytes read */
    return num_bytes;
}

/* Function: fn_length
 * Inputs: fname - the file name
 * Return Value: - the length of the file's name
 * Function: Modified version of strlen to account for names that don't end with the null character
 */
uint32_t fn_length(const int8_t* fname){
  uint32_t len = 0;
  while (fname[len] != '\0'){
      len++;
      if(len == MAX_NAME_LENGTH)
        break;
  }
  return len;
}
