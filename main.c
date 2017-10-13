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

struct disk *disk;
const char *sel; //Selector of function to execute
int *ft; //frame table
int page_faults = 0;
int *faulted_pages;

void page_fault_handler( struct page_table *pt, int page )
{
	int nframes = page_table_get_nframes(pt);
	int npages = page_table_get_npages(pt);
	page_faults++;
	char *data = page_table_get_physmem(pt);

	for (int i = 0; i < nframes; i++){
		if(ft[i] == -1) {
			page_table_set_entry(pt, page, i, PROT_READ|PROT_WRITE);
			disk_read(disk, page, &data[i*PAGE_SIZE]);
			ft[i] = page;
			return;
		}
	}
	int selected_page = -1;
	
	if (strcmp("rand", sel) == 0){
		
		while (selected_page == -1){
			selected_page = ft[lrand48()%nframes];
		}
	}
	
	else if (strcmp("fifo", sel) == 0){
		
		static int frame = 0; //the "first" updated frame.
		selected_page = ft[frame];
		frame ++;
		frame = frame%nframes;
	}
	
	else if (strcmp("custom", sel) == 0){
		
		faulted_pages[page] ++;
		int lower_faults_page_index = -1;
		int page_in_memory = 0;
		
		for (int i = 0; i < npages; i++){
			for (int j = 0; j < nframes; j++){
				if(ft[j] == i) page_in_memory = 1;
				else page_in_memory = 0;
			}
			if((faulted_pages[i] <= faulted_pages[lower_faults_page_index] && page_in_memory) || (lower_faults_page_index == -1 && page_in_memory)){
				lower_faults_page_index = i;
			}
		}
		selected_page = lower_faults_page_index;
	}
	
	else{
		printf("No valid algorithm selected\n");
		exit(1);
	}
	
	for (int i = 0; i < nframes; i++){
		if(ft[i] == selected_page){
			disk_write(disk, selected_page, &data[i*PAGE_SIZE]);
			page_table_set_entry(pt, page, i, PROT_READ|PROT_WRITE);
		}
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
	for (int i = 0; i < nframes; i++){
		ft[i] = -1;
	}
	faulted_pages = malloc(npages*sizeof(int));
	for (int i = 0; i < npages; i++){
		faulted_pages[i] = 0;
	} 
	
	disk = disk_open("myvirtualdisk",npages);
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
