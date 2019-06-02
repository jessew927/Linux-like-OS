#include "page.h"
#include "lib.h"
#include "x86_desc.h"

/* global arrays for the page directory and page table */
PDE_t __attribute__((aligned (4096))) page_directory[MAX_ENTRIES];
PTE_t __attribute__((aligned (4096))) vidmem_page_table[MAX_ENTRIES];
PTE_t __attribute__((aligned (4096))) user_vidmem_page_table[MAX_ENTRIES];

void init_page(void){
    /* for loop indices */
    int i, j;

    /* For details of the default page values, refer to IA-32 page 3-25 */

    /* initialize the page table to default values*/
    page_directory[0].table_PDE.present = 0x1;
    page_directory[0].table_PDE.read_write = 0x1;
    page_directory[0].table_PDE.user_super = 0x0;
    page_directory[0].table_PDE.pwt = 0x0;
    page_directory[0].table_PDE.pcd = 0x0;
    page_directory[0].table_PDE.accessed = 0x0;
    page_directory[0].table_PDE.reserved = 0x0;
    page_directory[0].table_PDE.page_size = 0x0;
    page_directory[0].table_PDE.global = 0x0;
    page_directory[0].table_PDE.available = 0x0;
    page_directory[0].table_PDE.table_addr = (uint32_t)vidmem_page_table >> ADDRESS_SHIFT;

    /* initialize the kernel page to default values*/
    page_directory[1].page_PDE.present = 0x1;
    page_directory[1].page_PDE.read_write = 0x1;
    page_directory[1].page_PDE.user_super = 0x1;
    page_directory[1].page_PDE.pwt = 0x0;
    page_directory[1].page_PDE.pcd = 0x0;
    page_directory[1].page_PDE.accessed = 0x0;
    page_directory[1].page_PDE.dirty = 0x0;
    page_directory[1].page_PDE.page_size = 0x1;
    page_directory[1].page_PDE.global = 0x1;
    page_directory[1].page_PDE.available = 0x0;
    page_directory[1].page_PDE.pat = 0x0;
    page_directory[1].page_PDE.reserved = 0x0;
    page_directory[1].page_PDE.page_addr = 0x1;

    /* fill in the page table to default values, mark everything that isnt vid_mem
    * as not present
    */
    for(i = 0; i < MAX_ENTRIES; i++){
      /* if the current mapping is to the Video memory
      * then mark present, else mark it unpresent */
      if(i == VID_MEM_ADDR || i == BACKGROUND_1 || i == BACKGROUND_2 || i == BACKGROUND_3){
        vidmem_page_table[i].present = 0x1;
        vidmem_page_table[i].user_super = 0x1;
      }
      else{
        vidmem_page_table[i].present = 0x0;
        vidmem_page_table[i].user_super = 0x0;
      }
      vidmem_page_table[i].read_write = 0x1;
      vidmem_page_table[i].pwt = 0x0;
      vidmem_page_table[i].pcd = 0x0;
      vidmem_page_table[i].accessed = 0x0;
      vidmem_page_table[i].dirty = 0x0;
      vidmem_page_table[i].pat = 0x0;
      vidmem_page_table[i].global = 0x0;
      vidmem_page_table[i].available = 0x0;
      vidmem_page_table[i].page_addr = i;
    }

    for(i = 0; i < MAX_ENTRIES; i++){
      /* if the current mapping is to the Video memory
      * then mark present, else mark it unpresent */
      user_vidmem_page_table[i].read_write = 0x1;
      user_vidmem_page_table[i].pwt = 0x0;
      user_vidmem_page_table[i].pcd = 0x0;
      user_vidmem_page_table[i].accessed = 0x0;
      user_vidmem_page_table[i].dirty = 0x0;
      user_vidmem_page_table[i].pat = 0x0;
      user_vidmem_page_table[i].global = 0x0;
      user_vidmem_page_table[i].available = 0x0;
      user_vidmem_page_table[i].page_addr = i;
      if(i == 0){
        user_vidmem_page_table[i].present = 0x1;
        user_vidmem_page_table[i].user_super = 0x1;
        user_vidmem_page_table[i].page_addr = VID_MEM_ADDR;
      }
      else{
        user_vidmem_page_table[i].present = 0x0;
        user_vidmem_page_table[i].user_super = 0x0;
      }
    }

    /* mark the rest of the page directory to default values and mark as not present */
    for(j = 2; j < MAX_ENTRIES; j++){
      page_directory[j].page_PDE.present = 0x0;
      page_directory[j].page_PDE.read_write = 0x1;
      page_directory[j].page_PDE.user_super = 0x0;
      page_directory[j].page_PDE.pwt = 0x0;
      page_directory[j].page_PDE.pcd = 0x0;
      page_directory[j].page_PDE.accessed = 0x0;
      page_directory[j].page_PDE.dirty = 0x0;
      page_directory[j].page_PDE.page_size = 0x1;
      page_directory[j].page_PDE.global = 0x0;
      page_directory[j].page_PDE.available = 0x0;
      page_directory[j].page_PDE.pat = 0x0;
      page_directory[j].page_PDE.reserved =0x0;
      page_directory[j].page_PDE.page_addr = 0x0;
    }

    // Reserve a user page table entry so we can set it directly later
    page_directory[USER_PAGE_INDEX].page_PDE.present = 0x1;
    page_directory[USER_PAGE_INDEX].page_PDE.read_write = 0x1;
    page_directory[USER_PAGE_INDEX].page_PDE.user_super = 0x1;
    page_directory[USER_PAGE_INDEX].page_PDE.pwt = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.pcd = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.accessed = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.dirty = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.page_size = 0x1;
    page_directory[USER_PAGE_INDEX].page_PDE.global = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.available = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.pat = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.reserved = 0x0;
    page_directory[USER_PAGE_INDEX].page_PDE.page_addr = USER_PAGE_INDEX;

    page_directory[USER_VIDMEM_INDEX].table_PDE.present = 0x1;
    page_directory[USER_VIDMEM_INDEX].table_PDE.read_write = 0x1;
    page_directory[USER_VIDMEM_INDEX].table_PDE.user_super = 0x1;
    page_directory[USER_VIDMEM_INDEX].table_PDE.pwt = 0x0;
    page_directory[USER_VIDMEM_INDEX].table_PDE.pcd = 0x0;
    page_directory[USER_VIDMEM_INDEX].table_PDE.accessed = 0x0;
    page_directory[USER_VIDMEM_INDEX].table_PDE.page_size = 0x0;
    page_directory[USER_VIDMEM_INDEX].table_PDE.global = 0x0;
    page_directory[USER_VIDMEM_INDEX].table_PDE.available = 0x0;
    page_directory[USER_VIDMEM_INDEX].table_PDE.reserved = 0x0;
    page_directory[USER_VIDMEM_INDEX].table_PDE.table_addr = (uint32_t)user_vidmem_page_table >> ADDRESS_SHIFT;

    /* Enable paging and 4MB pages */
    asm volatile(
      " movl %0, %%eax; "
      " movl %%eax, %%cr3; "
      " movl %%cr4, %%eax; "
      " orl $0x00000010, %%eax; "
      " movl %%eax, %%cr4; "
      " movl %%cr0, %%eax; "
      " orl $0x80000001, %%eax; "
      " movl %%eax, %%cr0; "
      :
      : "r"(page_directory)
      :"%eax"
    );

}
