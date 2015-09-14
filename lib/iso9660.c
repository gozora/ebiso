/*
 * iso9660.c
 * 
 * Version:       0.0.2-alfa
 * 
 * Release date:  08.09.2015
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

uint32_t iso9660_header(void **header, struct file_list_t file_list, struct ISO_data_t ISO_data) {
   void *rr_header = (void *) malloc(BLOCK_SIZE);
   void *header_start = rr_header;
   int offset = ISO_data.path_table_size / (BLOCK_SIZE + 1);
   uint32_t header_size = 0;
   time_t time_now = time(NULL);

   char system_identifier[32] = VOLUME_ID;                                    // "LINUX"
   char volume_identifier[32] = SYSTEM_ID;                                    // "CDROM"
   uint64_t volume_space_size = get_int32_LSB_MSB(ISO_data.LBA_last);         // Size of whole ISO image in blocks ( * BLOCK_SIZE = ISO size [B])
   uint32_t volume_set_size = get_int16_LSB_MSB(one);
   uint32_t volume_sequence_number = get_int16_LSB_MSB(one);
   uint32_t logical_block_size = get_int16_LSB_MSB(BLOCK_SIZE);
   uint64_t path_table_size = get_int32_LSB_MSB(ISO_data.path_table_size);    // Path table size
   uint32_t path_table_LBA = 0x14;                                            // LBA of type-L path table (little endian)
   uint32_t opath_table_LBA = 0x00;                                           // LBA of type-L optional path table (little endian)
   uint32_t path_table_LBA_BE = __bswap_32(0x16 + offset);                    // LBA of type-M path table (big endian)
   uint32_t opath_table_LBA_BE = 0x00;                                        // LBA of type-M optional path table (big endian)
   
   /* Might be used more in future ... */
   char volume_set_id[128];
   char publisher_id[128];
   char data_preparer_id[128];
   char application_id[128] = "EBISO UEFI BOOTABLE ISO CREATOR (C) 2015 V. (SODOMA) GOZORA";
   char copyright_file_id[38];
   char abstract_file_id[36];
   char biblio_file_id[37];
   
   /* Date/times */
   char date_time[17];
   char expiration_date_time[17];
   
   memset(rr_header, 0, BLOCK_SIZE);
   
   /* 
    * According to ISO9660 unused space should be filled
    * with (0x20) ASCII spaces ' '
    */
   str_var_prepare(system_identifier, 0x20, sizeof(system_identifier));
   str_var_prepare(volume_identifier, 0x20, sizeof(volume_identifier));
   str_var_prepare(application_id, 0x20, sizeof(application_id));
   
   memset(volume_set_id, 0x20, sizeof(volume_set_id));
   memset(publisher_id, 0x20, sizeof(publisher_id));
   memset(data_preparer_id, 0x20, sizeof(data_preparer_id));
   memset(copyright_file_id, 0x20, sizeof(copyright_file_id));
   memset(abstract_file_id, 0x20, sizeof(abstract_file_id));
   memset(biblio_file_id, 0x20, sizeof(biblio_file_id));

   memset(expiration_date_time, '0', sizeof(expiration_date_time));
   
   iso9660_cp2heap(&rr_header, &one, sizeof(uint8_t), &header_size);                                  // Type code of primary volume descirptor 0x01
   iso9660_cp2heap(&rr_header, ISO9660_ID, strlen(ISO9660_ID), &header_size);                         // Standard Identifier 'CD001'
   iso9660_cp2heap(&rr_header, &one, sizeof(uint8_t), &header_size);                                  // Version always 0x01
   rr_header += 1, header_size += 1;                                                                  // Unused
   iso9660_cp2heap(&rr_header, system_identifier, sizeof(system_identifier), &header_size);           // "LINUX"
   iso9660_cp2heap(&rr_header, volume_identifier, sizeof(volume_identifier), &header_size);           // "CDROM"
   rr_header += 8, header_size += 8;                                                                  // Unused
   iso9660_cp2heap(&rr_header, &volume_space_size, sizeof(uint64_t), &header_size);                   // Size of whole ISO image in blocks ( * BLOCK_SIZE = ISO size [B])
   rr_header += 32, header_size += 32;                                                                // Unused
   iso9660_cp2heap(&rr_header, &volume_set_size, sizeof(uint32_t), &header_size);                     // 0x01
   iso9660_cp2heap(&rr_header, &volume_sequence_number, sizeof(uint32_t), &header_size);              // 0x01
   iso9660_cp2heap(&rr_header, &logical_block_size, sizeof(uint32_t), &header_size);                  // ISO file block size (BLOCK_SIZE)
   iso9660_cp2heap(&rr_header, &path_table_size, sizeof(uint64_t), &header_size);                     // Size of path table
   iso9660_cp2heap(&rr_header, &path_table_LBA, sizeof(uint32_t), &header_size);                      // Path table will always start on block 0x14 (hopefully)
   iso9660_cp2heap(&rr_header, &opath_table_LBA, sizeof(uint32_t), &header_size);                     // Location of optional path table
   iso9660_cp2heap(&rr_header, &path_table_LBA_BE, sizeof(uint32_t), &header_size);                   // type-M path table (big endian)
   iso9660_cp2heap(&rr_header, &opath_table_LBA_BE, sizeof(uint32_t), &header_size);                  // LBA of type-M optional path table (big endian)
   
   /* Create root directory record for header */
   construct_dir_segment(&file_list, &rr_header, ROOT_HEADER);
   
   iso9660_cp2heap(&rr_header, volume_set_id, sizeof(volume_set_id), &header_size);
   iso9660_cp2heap(&rr_header, publisher_id, sizeof(publisher_id), &header_size);
   iso9660_cp2heap(&rr_header, data_preparer_id, sizeof(data_preparer_id), &header_size);
   iso9660_cp2heap(&rr_header, application_id, sizeof(application_id), &header_size);                 // So far, only this one is in use
   iso9660_cp2heap(&rr_header, copyright_file_id, sizeof(copyright_file_id), &header_size);
   iso9660_cp2heap(&rr_header, abstract_file_id, sizeof(abstract_file_id), &header_size);
   iso9660_cp2heap(&rr_header, biblio_file_id, sizeof(biblio_file_id), &header_size);
   
   /* If conversion fails, use default values ('0') */
   if (format_header_date(time_now, date_time) != E_OK) {
      printf("Warning: iso9660_header(): Failed to convert creation date/time\n");
      memset(date_time, '0', sizeof(date_time));
   }
   
   iso9660_cp2heap(&rr_header, date_time, sizeof(date_time), &header_size);                           // Creation date/time
   iso9660_cp2heap(&rr_header, date_time, sizeof(date_time), &header_size);                           // Modification date/time
   iso9660_cp2heap(&rr_header, expiration_date_time, sizeof(expiration_date_time), &header_size);     // Expiration date/tim
   iso9660_cp2heap(&rr_header, date_time, sizeof(date_time), &header_size);                           // Effective date/time
   
   iso9660_cp2heap(&rr_header, &one, sizeof(uint8_t), &header_size);                                  // File structure version
   iso9660_cp2heap(&rr_header, &zero, sizeof(uint8_t), &header_size);                                 // Unused
   
   *header = header_start;
   return header_size;
}

int iso9660_path_table(struct file_list_t *file_list, void **path_table, enum endianity_l endianity, struct ISO_data_t *ISO_data) {
   struct file_list_t *rr_file_list = file_list;
   void *rr_path_table = (void *) malloc(BLOCK_SIZE);
   void *path_table_start = rr_path_table;
   void *tmp_realloc = NULL;
   int entry_len = 0;
   int count = 1;
   int mem_free = BLOCK_SIZE;
   int rv = 0;
   uint32_t path_table_size = 0;
   uint32_t LBA = 0;
   uint16_t parent_id = 0;
   uint8_t pad_len = 0;
   
   memset(rr_path_table, 0, BLOCK_SIZE);
   
   while(rr_file_list->next != NULL ) {
      if (S_ISDIR(rr_file_list->st_mode)) {
         pad_len = do_pad(rr_file_list->name_short_len, PAD_ODD);
         
         /* Switch endianity for selected parematers */ 
         LBA = rr_file_list->LBA;
         parent_id = rr_file_list->parent_id;
         if (endianity == MSB) {
            LBA = __bswap_32(LBA);
            parent_id = __bswap_16(parent_id);
         }
         
         /* Dynamic memory allocation BEGIN */
         entry_len = 8 + rr_file_list->name_short_len + pad_len;
         mem_free -= entry_len;
         if (mem_free < 0) {
            count++;
            
            if ((tmp_realloc = realloc(path_table_start, count * BLOCK_SIZE)) != NULL) {
               path_table_start = tmp_realloc;
               mem_free = BLOCK_SIZE + (BLOCK_SIZE - path_table_size);
               memset(path_table_start + BLOCK_SIZE, 0, BLOCK_SIZE);
               rr_path_table = path_table_start + path_table_size;
            }
            else {
               printf("Error: iso9660_path_table(): Memory allocation failed\n");
               rv = E_MALLOC;
               break;
            }
         }
         /* Dynamic memory allocation END */
         
         iso9660_cp2heap(&rr_path_table, &rr_file_list->name_short_len, sizeof(uint8_t), &path_table_size);                     // Len of dir name
         iso9660_cp2heap(&rr_path_table, &zero, sizeof(uint8_t), &path_table_size);                                             // Extended attr record
         iso9660_cp2heap(&rr_path_table, &LBA, sizeof(uint32_t), &path_table_size);                                             // LBA
         iso9660_cp2heap(&rr_path_table, &parent_id, sizeof(uint16_t), &path_table_size);                                       // Parent dir ID
         iso9660_cp2heap(&rr_path_table, rr_file_list->name_short, rr_file_list->name_short_len + pad_len, &path_table_size);   // Dir name
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   if (ISO_data != NULL)
      ISO_data->path_table_size = path_table_size;
   
   *path_table = path_table_start;
   
   return rv;
}

int iso9660_assign_LBA(struct file_list_t *file_list, struct ISO_data_t *ISO_data) {
   struct file_list_t *rr_file_list = file_list;
   struct file_list_t *tmp_file_list = NULL;
   struct file_list_t *search_result = NULL;
   int entry_len = 0;
   uint32_t LBA = LBA_ROOT + (2 * ISO_data->path_table_offset);      // Move first LBA if path tables are larger that BLOCK_SIZE
   uint8_t pad_len = 0;
   
   /* Assign LBA to dirs first */
   while(rr_file_list->next != NULL) {
      if (S_ISDIR(rr_file_list->st_mode)) {
         
         tmp_file_list = file_list;
         entry_len = 0;
         
         /* Calculate how much blocks will parent directory occupy */
         while(tmp_file_list->next != NULL) {
            if (tmp_file_list->parent_id == rr_file_list->dir_id) {
               pad_len = do_pad(tmp_file_list->name_short_len, PAD_EVEN);
               entry_len += DIR_RECORD_LEN + tmp_file_list->name_short_len + pad_len;
            }
            
            tmp_file_list = tmp_file_list->next;
         }
         
         rr_file_list->blocks = (entry_len / BLOCK_SIZE) + 1;
         rr_file_list->LBA = LBA;
         LBA += rr_file_list->blocks;
         ISO_data->dir_count++;
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   /* Assign first 2 file LBAs to BOOT.CAT and UEFI boot image */
   search_result = list_search(file_list, ISO_data->boot_cat_file);
   search_result->LBA = LBA;
   ISO_data->boot_cat_LBA = LBA;
   LBA += blocks_count(search_result->size);
   
   search_result = list_search(file_list, ISO_data->efi_boot_file_full);
   search_result->LBA = LBA;
   LBA += blocks_count(search_result->size);
   
   /* Assign LBA to files */
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
   void *tmp_realloc = NULL;
   struct file_list_t *rr_file_list = file_list;
   struct file_list_t *tmp_file_list = NULL;
   int rv = 0;
   int count = 1;
   int entry_len = 0;
   int bytes_written = 0;
   uint8_t file_terminator_len = 0;
   uint8_t pad = 0;
   
   while(rr_file_list->next != NULL) {
      /* Skip files */
      if (!S_ISDIR(rr_file_list->st_mode)) {
         rr_file_list = rr_file_list->next;
         continue;
      }
      
      /* Reset pointer and clean memory block */
      directory_table = directory_table_start;
      memset(directory_table_start, 0, BLOCK_SIZE);

      /* Create ROOT record for CURRENT directory */
      bytes_written = construct_dir_segment(rr_file_list, &directory_table, ROOT);
      
      /*
       * Search whole file list from beginning
       * Search for all elements (dirs, files, ...), with matching parent ID
       */
      tmp_file_list = file_list;
      while(tmp_file_list->next != NULL) {
         /*
          * Root entry in list was artificially created.
          * Following condition will skip it
          */
         if (tmp_file_list->parent_id == rr_file_list->dir_id && *tmp_file_list->name_short != 0 ) {
            
            /* Dynamic memory allocataion block BEGIN */
            if ( (tmp_file_list->name_short_len % 2) == 0)
               pad = 1;
            else
               pad = 0;
            
            if (S_ISDIR(tmp_file_list->st_mode))
               file_terminator_len = 0;
            else
               file_terminator_len = 2;
            
            entry_len = DIR_RECORD_LEN + tmp_file_list->name_short_len + pad + file_terminator_len;
            
            if (bytes_written + entry_len > count * BLOCK_SIZE) {
               count++;
               if ((tmp_realloc = realloc(directory_table_start, count * BLOCK_SIZE)) != NULL) {
                  directory_table_start = tmp_realloc;
                  memset(directory_table_start + (count * BLOCK_SIZE) - BLOCK_SIZE, 0, BLOCK_SIZE);
                  
                  /* 
                   * Directory table can't cross block borders.
                   * Dont mess up with couple of free bytes and move pointer to new block
                   */
                  directory_table = directory_table_start + (count * BLOCK_SIZE) - BLOCK_SIZE;
                  bytes_written = (count - 1) * BLOCK_SIZE;
               }
               else {
                  printf("Error: iso9660_directory(): Memory allocation failed\n");
                  rv = E_MALLOC;
                  goto cleanup;
               }
            }
            /* Dynamic memory allocataion block END */
            
            bytes_written += construct_dir_segment(tmp_file_list, &directory_table, OTHER);
         }
         
         tmp_file_list = tmp_file_list->next;
      }
      
      fwrite(directory_table_start, 1, count * BLOCK_SIZE, dest);
      
      if (count > 1) {
         count = 1;
         free(directory_table_start);
         directory_table_start = (void *) malloc(BLOCK_SIZE);
      }
      
      rr_file_list = rr_file_list->next;
   }
   
   rv = write_files(file_list, dest);

cleanup:
   free(directory_table_start);
   
   return rv;
}

static uint32_t construct_dir_segment(struct file_list_t *file_list, void **directory_table_output, enum segment_list_t type) {
   struct file_list_t *rr_file_list = file_list;
   struct tm *ts = NULL;
   void *rr_directory_table = *directory_table_output;
   void *rr_directory_table_start = rr_directory_table;
   int file_terminator_len = 0;
   uint64_t data_len = 0;
   uint64_t LBA = 0;
   uint32_t directory_table_size = 0;
   uint32_t volume_seq_number = get_int16_LSB_MSB(one);
   uint8_t length_dir_record = 0;
   uint8_t name_len = 0;
   uint8_t ext_attr_record = 0;
   uint8_t flags = 0;                                                // Will be adjusted in future, 0x02 indicates directory
   uint8_t pad_len = 0;
   char *file_terminator = ";1";
   char date_time[7];
   
   memset(date_time, 0, sizeof(date_time));
   
   ts = localtime(&file_list->mtime);
   
   /* Assign date/time values */
   date_time[0] = ts->tm_year;
   date_time[1] = ts->tm_mon + 1;
   date_time[2] = ts->tm_mday;
   date_time[3] = ts->tm_hour;
   date_time[4] = ts->tm_min;
   date_time[5] = ts->tm_sec;
   date_time[6] = ts->tm_gmtoff / 60 / 15;
   
   LBA = get_int32_LSB_MSB(rr_file_list->LBA);
   
   pad_len = do_pad(rr_file_list->name_short_len, PAD_EVEN);
   
   if (S_ISDIR(rr_file_list->st_mode)) {
      flags = 0x02;
      file_terminator_len = 0;
      data_len = get_int32_LSB_MSB(rr_file_list->blocks * BLOCK_SIZE);
   }
   else {
      flags = 0x00;
      file_terminator_len = 2;
      data_len = get_int32_LSB_MSB(rr_file_list->size);
   }
   
   if (type == ROOT || type == ROOT_HEADER) {
      name_len = 1;                                                                                   // Lenght of roor dir name is always 1
      length_dir_record = DIR_RECORD_LEN + name_len;
      pad_len = 0;
   }
   else {
      length_dir_record = DIR_RECORD_LEN + rr_file_list->name_short_len + file_terminator_len + pad_len;
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
   if (pad_len == 1)
      iso9660_cp2heap(&rr_directory_table, &zero, pad_len, &directory_table_size);
   
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

static int int2str(uint16_t input, char **output) {
   char *rr_output = *output;
   uint8_t result = 0;
   uint16_t i = 0;
   const uint8_t offset = 0x30;
   const uint8_t divide = 10;
   uint16_t max_val;
   
   if (input < 100)
      max_val = 99;
   else
      max_val = 9999;
   
   if (input > max_val || input < 0)
      return -1;
   
   for (i = (max_val + 1) / divide ; i != 1; i /= divide) {
      result = (input / i) + offset;
      *rr_output++ = result;
      
      input -= (i * (result - offset));
   }
   
   *rr_output = (input % divide) + offset;
   
   *output = rr_output + 1;
   return 0;
}

static int format_header_date(time_t time_now, char *output) {
   char *rr_output = output;
   struct tm *ts = NULL;
   ts = localtime(&time_now);
   
   if (int2str(ts->tm_year + 1900, &rr_output) != E_OK)     // 4 bytes - Year
      return E_CONV;
   
   if (int2str(ts->tm_mon + 1, &rr_output) != E_OK)         // 2 bytes - Month
      return E_CONV;
      
   if (int2str(ts->tm_mday, &rr_output) != E_OK)            // 2 bytes - Day
      return E_CONV;
   
   if (int2str(ts->tm_hour, &rr_output) != E_OK)            // 2 bytes - Hour
      return E_CONV;
      
   if (int2str(ts->tm_min, &rr_output) != E_OK)             // 2 bytes - Min
      return E_CONV;
      
   if (int2str(ts->tm_sec, &rr_output) != E_OK)             // 2 bytes - Sec
      return E_CONV;
      
   if (int2str(00, &rr_output) != E_OK)                     // 2 bytes - Hundredths of a second from 0 to 99
      return E_CONV;
   
   *rr_output = ts->tm_gmtoff / 60 / 15;                    // 1 byte - Timezone offset from GMT in 15 minute intervals
   
   return E_OK;
}

static void str_var_prepare(char *input, char fill_char, size_t input_size) {
   size_t input_len = strlen(input);
   
   input += input_len;
   memset(input, fill_char, input_size - input_len);
}

static uint8_t do_pad(uint8_t len, enum pad_list_t type) {
   uint8_t pad_len = 0;
   
   if ((type == PAD_EVEN && len % 2 == 0) || (type == PAD_ODD && len % 2 != 0))
      pad_len = 1;
   
   return pad_len;
}
