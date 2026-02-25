#include <stdint.h>
#include "fat.h"
#include "ide.h"

#define PARTITION_LBA 2048u
#define MAX_ROOT_ENTRIES 512

struct boot_sector bs;

uint32_t fat_start_lba;
uint32_t root_start_lba;
uint32_t data_start_lba;

struct root_directory_entry root_dir[MAX_ROOT_ENTRIES];
uint32_t root_dir_sectors;

uint8_t sector_buf[SECTOR_SIZE];
uint8_t fat_sector_buf[SECTOR_SIZE];

uint8_t toUpper(uint8_t c) {

    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a' + 'A');
    return c;

}

uint32_t compare(uint32_t a, uint32_t b){

    return (a < b) ? a : b;

}

void driver_init(char *path_to_file){

    (void)path_to_file;

}

void sector_read(int sector_num, char *buf) {

    ata_lba_read(sector_num, (char*)buf, 1);

}

uint32_t compute_root_dir_sectors(uint16_t root_entries, uint16_t bytes_per_sector) {

    uint32_t bytes = (uint32_t)root_entries * 32u;
    uint32_t bps = (uint32_t)bytes_per_sector;
    return (bytes + (bps - 1u)) / bps;

}

uint16_t next_cluster(uint16_t cluster){

    uint32_t fat_offset = (uint32_t)cluster * 2u;
    uint32_t sector = fat_start_lba + (fat_offset / SECTOR_SIZE);
    uint32_t off = fat_offset % SECTOR_SIZE;

    sector_read((unsigned int)sector, (char*)fat_sector_buf);

    uint16_t lo = fat_sector_buf[off];
    uint32_t hi = fat_sector_buf[off +1u];
    return (uint16_t)(lo | (hi << 8));

}

int end_of_chain(uint16_t cluster){

    return cluster >= 0xFFF8u;

}

int deleted_or_free(struct root_directory_entry *rde) {

    uint8_t first = (uint8_t)rde->file_name[0];
    return (first == 0x00u) || (first == 0xE5u);

}

int is_lfn_entry(struct root_directory_entry *rde){

    return rde->attribute == 0x0Fu;

}

void format(char *path, char out_name[8], char ext[3]){

    for (int i = 0; i < 8; i++) out_name[i] = ' ';
    for (int i = 0; i < 3; i++) ext[i] = ' ';

    int i = 0;
    int j = 0;
    while (path[i] != 0 && path[i] != '.' && j < 8) {

        out_name[j++] = (char)toUpper((uint8_t)path[i]);
        i++;
        
    }

    if (path[i] == '.') {

        i++;
        j = 0;
        while (path[i] != 0 && j < 3) {

            ext[j++] = (char)toUpper((uint8_t)path[i]);
            i++;

        }

    }

}

int match(struct root_directory_entry *rde, char name[8], char ext[3]){

    for (int i = 0; i < 8; i++){

        if (rde->file_name[i] != name[i]) return 0;

    }

    for (int i = 0; i < 3; i++){

        if (rde->file_extension[i] != ext[i]) return 0;

    }

    return 1;

}

int fatInit() {

    sector_read(PARTITION_LBA, (char*)sector_buf);

    for (int i = 0; i < sizeof(struct boot_sector) && i < SECTOR_SIZE; i++){

        ((uint8_t*)&bs)[i] = sector_buf[i];

    }

    fat_start_lba = PARTITION_LBA + (uint32_t)bs.num_reserved_sectors;
    root_start_lba = PARTITION_LBA
        + (uint32_t)bs.num_reserved_sectors
        + (uint32_t)bs.num_fat_tables * (uint32_t)bs.num_sectors_per_fat;

    root_dir_sectors = compute_root_dir_sectors(bs.num_root_dir_entries, bs.bytes_per_sector);
    data_start_lba = root_start_lba + root_dir_sectors;

    uint32_t max_root_bytes = (uint32_t)MAX_ROOT_ENTRIES * 32u;
    uint32_t root_bytes = (uint32_t)bs.num_root_dir_entries * 32u;
    uint32_t bytes_to_read = (root_bytes > max_root_bytes) ? max_root_bytes : root_bytes;

    uint32_t sectors_to_read = (bytes_to_read + (SECTOR_SIZE - 1u)) / SECTOR_SIZE;
    if (sectors_to_read > root_dir_sectors) sectors_to_read = root_dir_sectors;

    uint8_t *dst = (uint8_t*)root_dir;
    for (uint32_t i = 0; i < sectors_to_read; i++) {

        sector_read((int)(root_start_lba + i), (char*)sector_buf);
        for (uint32_t j = 0; j < SECTOR_SIZE; j++){

            dst[i * SECTOR_SIZE + j] = sector_buf[j];

        }

    }

    return 0;

}

struct root_directory_entry *fatOpen(char *path){

    char name[8];
    char ext[3];
    format(path, name, ext);

    uint32_t entries = (uint32_t)bs.num_root_dir_entries;
    if (entries > MAX_ROOT_ENTRIES) entries = MAX_ROOT_ENTRIES;

    for (uint32_t i = 0; i < entries; i++) {

        struct root_directory_entry *rde = &root_dir[i];

        if ((uint8_t)rde->file_name[0] == 0x00u) break;

        if (deleted_or_free(rde)) continue;
        if (is_lfn_entry(rde)) continue;
        
        if (rde->attribute & FILE_ATTRIBUTE_SUBDIRECTORY) continue;

        if (match(rde, name, ext)) return (struct root_directory_entry*)rde;

    }

    return (struct root_directory_entry*)0;

}

int fatRead(struct root_directory_entry *rde_ptr, char *buf, int n){

    if (!rde_ptr || !buf || n <= 0) return 0;

    struct root_directory_entry *rde = (struct root_directory_entry*)rde_ptr;

    uint32_t to_read = compare((uint32_t)n, rde->file_size);
    uint16_t cluster = rde->cluster;
    uint32_t bytes_read = 0;

    if (cluster < 2u) return 0;

    while (!end_of_chain(cluster) && bytes_read < to_read) {

        uint32_t first_sector = data_start_lba
            +(uint32_t)(cluster - 2u) * (uint32_t)bs.num_sectors_per_cluster;

        for (uint32_t i = 0; i < (uint32_t)bs.num_sectors_per_cluster; i++){

            if (bytes_read >= to_read) break;
            
            sector_read((unsigned int)(first_sector + i), (char*)sector_buf);

            for (uint32_t j = 0; j < SECTOR_SIZE && bytes_read < to_read; j++) {

                buf[bytes_read++] = (char)sector_buf[j];

            }

        }

        cluster = next_cluster(cluster);

    }

    return (int)bytes_read;

}
