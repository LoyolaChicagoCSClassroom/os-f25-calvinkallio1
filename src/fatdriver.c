#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "fat.h"

int fd = 0;

void driver_init(char *disk_img_path){

	fd = open(disk_img_path, O_RDONLY);

}

void sector_read(unsigned int sector_num, char *buf){

	lseek(fd, sector_num * 512, SEEK_SET);
	size_t nbytes = read(fd, buf, 512);

}

char boot_sector[512];
char root_directory_region[512];
int main() {

    struct root_directory_entry *rde = ((struct root_directory_entry*)root_directory_region);
	driver_init("disk.img");
	sector_read(2048, boot_sector);
	
	printf("Sector size: %d\n", ((struct boot_sector*)boot_sector)->bytes_per_sector);
    printf("Sectors per cluster: %d\n", ((struct boot_sector*)boot_sector)->num_sectors_per_cluster);
    printf("Reserved sectors : %d\n", ((struct boot_sector*)boot_sector)->num_reserved_sectors);
    printf("FAT Tables: %d\n", ((struct boot_sector*)boot_sector)->num_fat_tables);
    printf("RDEs: %d\n", ((struct boot_sector*)boot_sector)->num_root_dir_entries);

    int root_dir_region_start = 2048
                                +((struct boot_sector*)boot_sector)->num_reserved_sectors
                                +((struct boot_sector*)boot_sector)->num_fat_tables 
                                * ((struct boot_sector*)boot_sector)->num_sectors_per_fat;

    printf("Root directory region start (sectors): %d\n", root_dir_region_start);

    sector_read(root_dir_region_start, root_directory_region);
    
    for (int i = 0; i < 10; i++){

        printf("File name \"%s.%s\"\n", rde[i].file_name, rde[i].file_extension);
        printf("Data cluster: %d\n", rde[i].cluster);
        printf("File size: %d\n", rde[i].file_size);
    }
	return 0;

}
