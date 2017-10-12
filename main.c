/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

const char *sel; //Selector of function to execute
int *ft; //frame table

void page_fault_handler( struct page_table *pt, int page )
{
	const char *data;
	static int ft_position = 0;
	printf("page fault on page #%d\n",page);
	if (strcmp("rand", sel) == 0){
		
	}
	else if (strcmp("fifo", sel) == 0){
		printf("fifo\n");
		int page_to_drop = page_table_get_frame_page(pt, ft[ft_position]);
		
		page_table_set_entry(pt, page_to_drop, ft[ft_position], NULL);
		page_table_set_entry(pt, page, ft[ft_position], PROT_READ|PROT_WRITE);
		page_table_print(pt);
		ft_position = (ft_position + 1)%page_table_get_nframes(pt);
	}
	else if (strcmp("custom", sel) == 0){
	}
	
	else{
		printf("No valid algorithm selected");
		exit(1);
	}
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}
	
	char *data;

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	sel = argv[3];
	const char *program = argv[4];
	
	ft = malloc(nframes*sizeof(int)); //we create the frame table

	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);
	
	for ( int i = 0; i<nframes; i++){
		page_table_print(pt);
		printf("\n");
		page_table_set_entry(pt, i, i, PROT_READ|PROT_WRITE);//recieves pagetable, page, frame, protection bit
		ft[i] = i;
		printf("");
		if (i == npages-1) break;
	}
	
	for (int i = 0; i < npages; i++){
		data = malloc(sizeof(char));
		disk_write(disk, i, data);
	}
	
	//page_table_print(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
