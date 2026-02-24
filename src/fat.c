#include <stdint.h>
#include "fat.h"
#include "ide.h"

int fd = 0;

void driver_init(char *path_to_file){

   fd = ata_lba_read(0, path_to_file, 4); 

}


void sector_read(unsigned int sector_num, char *buf){

    

}

int fatInit(){

	
}

struct rde * fatOpen(char *path){

    

}

int fatRead(struct rde *rde, char *buf, int n){



}
