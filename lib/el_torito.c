/*
 * el_torito.c
 * 
 * Version:       0.0.1-alfa
 * 
 * Release date:  07.09.2015
 * 
 * Copyright 2015 Vladimir (sodoma) Gozora <c@gozora.sk>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "el_torito.h"

void et_boot_record_descr(void **boot_record_descriptor, struct ISO_data_t ISO_data) {
   void *rr_boot_descriptor = *boot_record_descriptor;
   void *rr_boot_descriptor_start = rr_boot_descriptor;
   uint32_t desc_size = 0;
   char et_id[32];
   char unused[32];
   
   memset(et_id, 0, sizeof(et_id));
   memset(unused, 0, sizeof(unused));
   strcpy(et_id, "EL TORITO SPECIFICATION");
   
   iso9660_cp2heap(&rr_boot_descriptor, &zero, sizeof(uint8_t), &desc_size);           // Boot Record Indicator, always 0x00
   iso9660_cp2heap(&rr_boot_descriptor, ISO9660_ID, strlen(ISO9660_ID), &desc_size);   // ISO9660 identifier, always "CD001"
   iso9660_cp2heap(&rr_boot_descriptor, &one, sizeof(uint8_t), &desc_size);            // Version of this descriptor, always 0x01
   iso9660_cp2heap(&rr_boot_descriptor, et_id, sizeof(et_id), &desc_size);             // El-torito boot system ID
   iso9660_cp2heap(&rr_boot_descriptor, unused, sizeof(unused), &desc_size);           // zeros
   iso9660_cp2heap(&rr_boot_descriptor, &ISO_data.boot_cat_LBA, sizeof(uint32_t), &desc_size);  // Boot catalog LBA
   
   *boot_record_descriptor = rr_boot_descriptor_start;
}

void et_boot_catalog(struct ISO_data_t ISO_data, char *workdir) {
   void *boot_cat_data = (void *) malloc(BLOCK_SIZE);
   void *boot_cat_start = boot_cat_data;
   void *rr = NULL;
   struct stat EFI_boot_file_stat;
   char EFI_boot_file[MAX_DIR_STR_LEN];
   char boot_catalog_file[MAX_DIR_STR_LEN];
   
   uint32_t boot_cat_size = 0;
   FILE *boot_catalog = NULL;
   uint8_t header_id = 0x01;     // Always 0x01
   uint8_t platform_id = 0x00;   // x86
   uint16_t unused = 0x0000;
   char id_string[24];
   uint16_t checksum = 0x0000;
   uint16_t signature = 0xAA55;
   
   uint8_t boot_indicator = 0x88;   // Bootable
   uint8_t media_type = 0x00;       // No Emulation
   uint16_t load_segment = 0x00;
   uint8_t system_type = 0x00;
   uint8_t unused1 = 0x00;
   uint16_t sector_count = 0x00;
   
   memset(EFI_boot_file, 0, MAX_DIR_STR_LEN);
   memset(boot_catalog_file, 0, MAX_DIR_STR_LEN);
   memset(&EFI_boot_file_stat, 0, sizeof(struct stat));
   memset(id_string, 0, sizeof(id_string));
   memset(boot_cat_start, 0, BLOCK_SIZE);
   
   snprintf(EFI_boot_file, MAX_DIR_STR_LEN, "%s/%s", workdir, ISO_data.efi_boot_file);
   snprintf(boot_catalog_file, MAX_DIR_STR_LEN, "%s/%s", workdir, "BOOT.CAT");
   
   (ISO_data.boot_cat_LBA)++;
   
   stat(EFI_boot_file, &EFI_boot_file_stat);
   sector_count = EFI_boot_file_stat.st_size / 512;
   
   iso9660_cp2heap(&boot_cat_data, &header_id, sizeof(uint8_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &platform_id, sizeof(uint8_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &unused, sizeof(uint16_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &id_string, sizeof(id_string), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &checksum, sizeof(uint16_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &signature, sizeof(uint16_t), &boot_cat_size);
   
   /* Calculate checksum of words so far written */
   checksum = et_create_checksum(boot_cat_start, boot_cat_size);
   rr = boot_cat_data - 4;             // Return back 4 bytes and write checksum
   boot_cat_size -= sizeof(uint16_t);  // This will be counted back in next step
   iso9660_cp2heap(&rr, &checksum, sizeof(uint16_t), &boot_cat_size);
   
   iso9660_cp2heap(&boot_cat_data, &boot_indicator, sizeof(uint8_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &media_type, sizeof(uint8_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &load_segment, sizeof(uint16_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &system_type, sizeof(uint8_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &unused1, sizeof(uint8_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &sector_count, sizeof(uint16_t), &boot_cat_size);
   iso9660_cp2heap(&boot_cat_data, &ISO_data.boot_cat_LBA, sizeof(uint32_t), &boot_cat_size);
   
   boot_catalog = fopen(boot_catalog_file, "w");
   fwrite(boot_cat_start, 1, BLOCK_SIZE, boot_catalog);
   
   fclose(boot_catalog);
   free(boot_cat_start);
}

static uint16_t et_create_checksum(void *data, uint32_t data_size) {
   uint16_t *rr_val = data;
   uint16_t sum = 0;
   int size = data_size / sizeof(uint16_t);
   
   while(size--)
      sum += *(rr_val++);
   
   return 0xFFFF - sum + 1;
}
