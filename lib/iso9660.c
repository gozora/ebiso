/*
 * iso9660.c
 * 
 * Version:       0.0.2-alfa
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

#include "iso9660.h"

void iso9660_cp2heap(void **dest, const void *source, long int size, uint32_t *dest_size) {
   memcpy(*dest, source, size);
   *dest_size += size;
   *dest += size;
}

uint32_t iso9660_terminator(void **terminator) {
   void *rr_terminator = (void *) malloc(BLOCK_SIZE);
   void *terminator_start = rr_terminator;
   uint32_t terminator_size = 0;
   
   uint8_t type = 0xff;
   char identifier[5] = ISO9660_ID;
   uint8_t version = 0x01;
   
   memset(rr_terminator, 0, BLOCK_SIZE);
   
   iso9660_cp2heap(&rr_terminator, &type, sizeof(uint8_t), &terminator_size);
   iso9660_cp2heap(&rr_terminator, identifier, sizeof(identifier), &terminator_size);
   iso9660_cp2heap(&rr_terminator, &version, sizeof(uint8_t), &terminator_size);
   
   *terminator = terminator_start;
   return terminator_size;
}

uint32_t iso9660_header(uint32_t pt_size, void **header, struct file_list_t file_list, int LBA_last) {
   void *rr_header = (void *) malloc(BLOCK_SIZE);
   void *header_start = rr_header;

   uint32_t header_size = 0;
   uint8_t type_code = 0x01;                                                  // Stat
   uint8_t version = 0x01;                                                    // Stat
   char standard_identifier[5] = ISO9660_ID;                                  // "CD001"
   char volume_identifier[32] = SYSTEM_ID;                                    // "LINUX"
   char system_identifier[32] = VOLUME_ID;                                    // "CDROM"
   uint64_t volume_space_size = get_int32_LSB_MSB(LBA_last);                  // Number of blocks iso occupies ( * BLOCK_SIZE = iso file size)
   uint32_t volume_set_size = get_int16_LSB_MSB(0x01);                        // Stat
   uint32_t volume_sequence_number = get_int16_LSB_MSB(0x01);                 // Stat
   uint32_t logical_block_size = get_int16_LSB_MSB(BLOCK_SIZE);               // Stat
   uint64_t path_table_size = get_int32_LSB_MSB(pt_size);                     // From iso9660_path_table
   uint32_t path_table_LBA = 0x13;                                            // Currently hard coded
   uint32_t opath_table_LBA = 0x00;                                           // Nothing here
   uint32_t path_table_LBA_BE = __bswap_32(0x15);
   uint32_t opath_table_LBA_BE = 0x00;
   
   char volume_set_id[128];
   char publisher_id[128];
   char data_preparer_id[128];
   char application_id[128] = "GOISO";
   char copyright_file_id[38];
   char abstract_file_id[36];
   char biblio_file_id[37];
   char creation_date_time[17];
   char modification_date_time[17];
   char expiration_date_time[17];
   char effective_date_time[17];
   
   memset(rr_header, 0, BLOCK_SIZE);
   memset(system_identifier + strlen(system_identifier), 0x20, sizeof(system_identifier) - strlen(system_identifier));
   memset(volume_identifier + strlen(volume_identifier), 0x20, sizeof(volume_identifier) - strlen(volume_identifier));
   memset(volume_set_id, 0x20, sizeof(volume_set_id));
   memset(publisher_id, 0x20, sizeof(publisher_id));
   memset(data_preparer_id, 0x20, sizeof(data_preparer_id));
   memset(application_id + strlen(application_id), 0x20, sizeof(application_id) - strlen(application_id));
   memset(copyright_file_id, 0x20, sizeof(copyright_file_id));
   memset(abstract_file_id, 0x20, sizeof(abstract_file_id));
   memset(biblio_file_id, 0x20, sizeof(biblio_file_id));
   memset(creation_date_time, 0x20, sizeof(creation_date_time));
   memset(modification_date_time, 0x20, sizeof(modification_date_time));
   memset(expiration_date_time, 0x20, sizeof(expiration_date_time));
   memset(effective_date_time, 0x20, sizeof(effective_date_time));
   
   iso9660_cp2heap(&rr_header, &type_code, sizeof(uint8_t), &header_size);
   iso9660_cp2heap(&rr_header, standard_identifier, sizeof(standard_identifier), &header_size);
   iso9660_cp2heap(&rr_header, &version, sizeof(uint8_t), &header_size);
   rr_header += 1, header_size += 1;
   iso9660_cp2heap(&rr_header, system_identifier, sizeof(system_identifier), &header_size);
   iso9660_cp2heap(&rr_header, volume_identifier, sizeof(volume_identifier), &header_size);
   rr_header += 8, header_size += 8;
   iso9660_cp2heap(&rr_header, &volume_space_size, sizeof(uint64_t), &header_size);
   rr_header += 32, header_size += 32;
   iso9660_cp2heap(&rr_header, &volume_set_size, sizeof(uint32_t), &header_size);
   iso9660_cp2heap(&rr_header, &volume_sequence_number, sizeof(uint32_t), &header_size);
   iso9660_cp2heap(&rr_header, &logical_block_size, sizeof(uint32_t), &header_size);
   iso9660_cp2heap(&rr_header, &path_table_size, sizeof(uint64_t), &header_size);
   iso9660_cp2heap(&rr_header, &path_table_LBA, sizeof(uint32_t), &header_size);
   iso9660_cp2heap(&rr_header, &opath_table_LBA, sizeof(uint32_t), &header_size);
   iso9660_cp2heap(&rr_header, &path_table_LBA_BE, sizeof(uint32_t), &header_size);
   iso9660_cp2heap(&rr_header, &opath_table_LBA_BE, sizeof(uint32_t), &header_size);
   
   /* Create root directory record for header */
   construct_dir_segment(&file_list, &rr_header, ROOT_HEADER);
   
   iso9660_cp2heap(&rr_header, volume_set_id, sizeof(volume_set_id), &header_size);
   iso9660_cp2heap(&rr_header, publisher_id, sizeof(publisher_id), &header_size);
   iso9660_cp2heap(&rr_header, data_preparer_id, sizeof(data_preparer_id), &header_size);
   iso9660_cp2heap(&rr_header, application_id, sizeof(application_id), &header_size);
   iso9660_cp2heap(&rr_header, copyright_file_id, sizeof(copyright_file_id), &header_size);
   iso9660_cp2heap(&rr_header, abstract_file_id, sizeof(abstract_file_id), &header_size);
   iso9660_cp2heap(&rr_header, biblio_file_id, sizeof(biblio_file_id), &header_size);
   iso9660_cp2heap(&rr_header, creation_date_time, sizeof(creation_date_time), &header_size);
   iso9660_cp2heap(&rr_header, modification_date_time, sizeof(modification_date_time), &header_size);
   iso9660_cp2heap(&rr_header, expiration_date_time, sizeof(expiration_date_time), &header_size);
   iso9660_cp2heap(&rr_header, effective_date_time, sizeof(effective_date_time), &header_size);
   iso9660_cp2heap(&rr_header, &version, sizeof(uint8_t), &header_size);
   iso9660_cp2heap(&rr_header, &zero, sizeof(uint8_t), &header_size);
   
   *header = header_start;
   return header_size;
}

uint32_t iso9660_path_table(struct file_list_t *file_list, void **path_table) {
   struct file_list_t *rr_file_list = file_list;
   void *rr_path_table = (void *) malloc(BLOCK_SIZE);
   void *path_table_start = rr_path_table;
   int pad_len = 0;
   uint32_t path_table_size = 0;
   
   memset(rr_path_table, 0, BLOCK_SIZE);
   
   while(rr_file_list->next != NULL ) {
      if (S_ISDIR(rr_file_list->st_mode)) {
         if (rr_file_list->name_short_len % 2 != 0)
            pad_len = 1;
         else
            pad_len = 0;
         
         iso9660_cp2heap(&rr_path_table, &rr_file_list->name_short_len, sizeof(uint8_t), &path_table_size);                     // Len of dir name
         iso9660_cp2heap(&rr_path_table, &zero, sizeof(uint8_t), &path_table_size);                                             // Extended attr record
         iso9660_cp2heap(&rr_path_table, &rr_file_list->LBA, sizeof(uint32_t), &path_table_size);                               // LBA
         iso9660_cp2heap(&rr_path_table, &rr_file_list->parent_id, sizeof(uint16_t), &path_table_size);                         // Parent dir ID
         iso9660_cp2heap(&rr_path_table, rr_file_list->name_short, rr_file_list->name_short_len + pad_len, &path_table_size);   // Dir name
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   *path_table = path_table_start;
   return path_table_size;
}

int iso9660_assign_LBA(struct file_list_t *file_list, struct ISO_data_t *ISO_data) {
   struct file_list_t *rr_file_list = file_list;
   uint32_t LBA = LBA_ROOT;
   
   /*
    * Assign LBA to dirs first
    */
   while(rr_file_list->next != NULL) {
      if (S_ISDIR(rr_file_list->st_mode)) {
         rr_file_list->LBA = LBA++;
         ISO_data->dir_count++;
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   /*
    * Assign first 2 file LBAs to BOOT.CAT and efiboot.img
    * Find some better way to do this ....
    */
   rr_file_list = file_list;
   while(rr_file_list->next != NULL) {
      if (strncmp(rr_file_list->name_short, "BOOT.CAT", 8) == 0) {
         rr_file_list->LBA = LBA;
         ISO_data->boot_cat_LBA = LBA;
         LBA = LBA + blocks_count(rr_file_list->size);
         break;
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   rr_file_list = file_list;
   while(rr_file_list->next != NULL) {
      if (strncmp(rr_file_list->name_path, ISO_data->efi_boot_file_full, strlen(ISO_data->efi_boot_file_full)) == 0) {
         rr_file_list->LBA = LBA;
         LBA = LBA + blocks_count(rr_file_list->size);
         break;
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   /*
    * Assign LBA to files
    */
   rr_file_list = file_list;
   while(rr_file_list->next != NULL) {
      if (!S_ISDIR(rr_file_list->st_mode) && strncmp(rr_file_list->name_short, "BOOT.CAT", 8) \
      && strncmp(rr_file_list->name_path, ISO_data->efi_boot_file_full, strlen(ISO_data->efi_boot_file_full)) ) {
         rr_file_list->LBA = LBA;
         
         LBA = LBA + blocks_count(rr_file_list->size);
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   ISO_data->LBA_last = LBA;
   return LBA;
}

int iso9660_directory(struct file_list_t *file_list, FILE *dest) {
   void *directory_table = (void *) malloc(BLOCK_SIZE);
   void *directory_table_start = directory_table;
   struct file_list_t *rr_file_list = file_list;
   struct file_list_t *tmp_file_list = NULL;
   int rv = 0;
   
   while(rr_file_list->next != NULL) {
      /*
       * Skip files
       */
      if (!S_ISDIR(rr_file_list->st_mode)) {
         rr_file_list = rr_file_list->next;
         continue;
      }
      
      directory_table = directory_table_start;        // Reset pointer
      memset(directory_table_start, 0, BLOCK_SIZE);   // Clean memory block

      /*
       * Create ROOT record for CURRENT directory
       */
      construct_dir_segment(rr_file_list, &directory_table, ROOT);
      
      /*
       * Search whole file list from beginning
       * Search for all elements (dirs, files, ...), with matching parent ID
       */
      tmp_file_list = file_list;
      while(tmp_file_list->next != NULL) {
         /*
          * First entry in list is artificially created.
          * Following condition will skip it
          */
         if (tmp_file_list->parent_id == rr_file_list->dir_id && *tmp_file_list->name_short != 0 )
            construct_dir_segment(tmp_file_list, &directory_table, OTHER);
         
         tmp_file_list = tmp_file_list->next;
      }
      
      fwrite(directory_table_start, BLOCK_SIZE, 1, dest);
      
      rr_file_list = rr_file_list->next;
   }
   
   rv = write_files(file_list, dest);
   
   free(directory_table_start);
   
   return rv;
}

static uint32_t construct_dir_segment(struct file_list_t *file_list, void **directory_table_output, enum segment_list_t type) {
   struct file_list_t *rr_file_list = file_list;
   void *rr_directory_table = *directory_table_output;
   void *rr_directory_table_start = rr_directory_table;
   uint32_t directory_table_size = 0;
   
   uint8_t length_dir_record = 0x21;
   uint8_t ext_attr_record = 0x00;
   uint64_t LBA = get_int32_LSB_MSB(LBA_ROOT);
   uint64_t data_len = get_int32_LSB_MSB(BLOCK_SIZE);                // Must be adjusted
   char date_time[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};   // Must be adjusted
   uint8_t flags = 0x02;                                             // Must be adjuster, 0x02 indicates directory
   uint32_t volume_seq_number = get_int16_LSB_MSB(0x01);
   uint8_t name_len = 0x01;
   int pad = 0;
   int file_terminator_len = 0;
   char *file_terminator = ";1";
   
   LBA = get_int32_LSB_MSB(rr_file_list->LBA);
   
   if ( (rr_file_list->name_short_len % 2) == 0)
      pad = 1;
   else
      pad = 0;
   
   if (S_ISDIR(rr_file_list->st_mode)) {
      flags = 0x02;
      file_terminator_len = 0;
      data_len = get_int32_LSB_MSB(BLOCK_SIZE);
   }
   else {
      flags = 0x00;
      file_terminator_len = 2;
      data_len = get_int32_LSB_MSB(rr_file_list->size);
   }
   
   if (type == ROOT || type == ROOT_HEADER) {
      length_dir_record = 0x22;
      name_len = 0x01;
      pad = 0;
   }
   else {
      length_dir_record = 0x21 + rr_file_list->name_short_len + file_terminator_len + pad;                   // Make 0x21 constant
      name_len = rr_file_list->name_short_len + file_terminator_len;
   }
   
   iso9660_cp2heap(&rr_directory_table, &length_dir_record, sizeof(uint8_t), &directory_table_size);  // Lenght of whole entry
   iso9660_cp2heap(&rr_directory_table, &ext_attr_record, sizeof(uint8_t), &directory_table_size);    // Ext. attr. record
   iso9660_cp2heap(&rr_directory_table, &LBA, sizeof(uint64_t), &directory_table_size);               // LBA
   iso9660_cp2heap(&rr_directory_table, &data_len, sizeof(uint64_t), &directory_table_size);          // Size of file/dir
   iso9660_cp2heap(&rr_directory_table, date_time, sizeof(date_time), &directory_table_size);         // Recording date/time
   iso9660_cp2heap(&rr_directory_table, &flags, sizeof(uint8_t), &directory_table_size);              // Flags
   iso9660_cp2heap(&rr_directory_table, &zero, sizeof(uint8_t), &directory_table_size);               // Interleaving
   iso9660_cp2heap(&rr_directory_table, &zero, sizeof(uint8_t), &directory_table_size);               // Interleaving
   iso9660_cp2heap(&rr_directory_table, &volume_seq_number, sizeof(uint32_t), &directory_table_size); // Volume sequence number
   iso9660_cp2heap(&rr_directory_table, &name_len, sizeof(uint8_t), &directory_table_size);           // File name len
   
   /* File name */
   if (type == ROOT || type == ROOT_HEADER)
      iso9660_cp2heap(&rr_directory_table, &zero, name_len, &directory_table_size);
   else
      iso9660_cp2heap(&rr_directory_table, rr_file_list->name_short, rr_file_list->name_short_len, &directory_table_size);

   /* If entry is file, add file terminator */
   if (!S_ISDIR(rr_file_list->st_mode))
      iso9660_cp2heap(&rr_directory_table, file_terminator, file_terminator_len, &directory_table_size);
   
   /* Do padding */
   if (pad == 1)
      iso9660_cp2heap(&rr_directory_table, &zero, pad, &directory_table_size);
   
   /* Create root dir entry one more time, with name 0x01 ... */
   if (type == ROOT) {
      iso9660_cp2heap(&rr_directory_table, rr_directory_table_start, directory_table_size - 1, &directory_table_size);
      iso9660_cp2heap(&rr_directory_table, &name_len, sizeof(uint8_t), &directory_table_size);
   }
   
   *directory_table_output = rr_directory_table;
   
   return directory_table_size;
}

static int blocks_count(int size) {
   int blocks = 0;
   
   if (size % BLOCK_SIZE == 0)
      blocks = size / BLOCK_SIZE;
   else
      blocks = (size / BLOCK_SIZE) + 1;
   
   if (blocks == 0)
      blocks = 1;
   
   return blocks;
}

static uint64_t get_int32_LSB_MSB(uint64_t input) {
    return input + __bswap_64(input);
}

static uint32_t get_int16_LSB_MSB(uint32_t input) {
    return input + __bswap_32(input);
}

