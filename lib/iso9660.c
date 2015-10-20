/*
 * iso9660.c
 * 
 * Version:       0.2.0
 * 
 * Release date:  20.10.2015
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

/* Rockridge */
unsigned char CE[28];
unsigned char ER[237];
unsigned char SP[7];
unsigned char RR[5];
unsigned char PX[44];               // 1<<0
unsigned char TF[26];               // 1<<7
unsigned char *NM;                  // 1<<3
const enum rrip_fields_t init_fields = RRIP_INIT_FIELDS;

void iso9660_cp2heap(void **dest, const void *source, long int size, uint32_t *dest_size) {
   memcpy(*dest, source, size);
   
   if (dest_size != NULL) {
      *dest_size += size;
      *dest += size;
   }
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
   construct_dir_record(&file_list, &rr_header, ROOT_HEADER);
   
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
         pad_len = do_pad(rr_file_list->name_conv_len, PAD_ODD);
         
         /* Switch endianity for selected parematers */ 
         LBA = rr_file_list->LBA;
         parent_id = rr_file_list->parent_id;
         if (endianity == MSB) {
            LBA = __bswap_32(LBA);
            parent_id = __bswap_16(parent_id);
         }
         
         /* Dynamic memory allocation BEGIN */
         entry_len = PT_RECORD_LEN + rr_file_list->name_conv_len + pad_len;
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
         
         iso9660_cp2heap(&rr_path_table, &rr_file_list->name_conv_len, sizeof(uint8_t), &path_table_size);                      // Len of dir name
         iso9660_cp2heap(&rr_path_table, &zero, sizeof(uint8_t), &path_table_size);                                             // Extended attr record
         iso9660_cp2heap(&rr_path_table, &LBA, sizeof(uint32_t), &path_table_size);                                             // LBA
         iso9660_cp2heap(&rr_path_table, &parent_id, sizeof(uint16_t), &path_table_size);                                       // Parent dir ID
         iso9660_cp2heap(&rr_path_table, rr_file_list->name_conv, rr_file_list->name_conv_len + pad_len, &path_table_size);     // Dir name
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
   struct CE_list_t CE_list;
   int total_len = 0;
   int dir_rec_len = 0;
   int counter = 1;
   int min_dir_len = 2 * (DIR_RECORD_LEN + 1);                       // Directory recod begins with two entries of directory it self
   int additional_bytes = 0;
   int rock_ridge_header_size = 0;
   int rv = E_OK;
   uint32_t LBA = LBA_ROOT + (2 * ISO_data->path_table_offset);      // Move first LBA if path tables are larger that BLOCK_SIZE
   uint8_t pad_len = 0;
   uint8_t terminator_len = 0;
   bool_t rock_ridge_on = FALSE;
   
   memset(&CE_list, 0, sizeof(CE_list));
   
   if (option_on_off(ISO_data->options, OPT_R) == E_OK) {
      rock_ridge_header_size = init_RRIP();
      pad_len = do_pad(rock_ridge_header_size, PAD_ODD);
      min_dir_len += (2 * rock_ridge_header_size) + (2 * pad_len);   // 2 records at the beginning of directory record
      rock_ridge_on = TRUE;
   }
   
   /* Initialize CE_list for directory records */
   if ((rv = CEarr_init_list(&CE_list, CE_LIST_PREALLOC)) != E_OK) {
      CEarr_destroy_list(&CE_list);
      return rv;
   }
   
   /* Assign LBAs to directories first */
   while(rr_file_list->next != NULL) {
      if (S_ISDIR(rr_file_list->st_mode)) {
         
         tmp_file_list = file_list;
         total_len = 0;
         counter = 1;
         dir_rec_len = min_dir_len;
         
         /* Absolute root directory will have SP & CE on top */
         if (strncmp(rr_file_list->name_path, ".", 2) == 0 && rock_ridge_on == TRUE) {
            dir_rec_len += (sizeof(SP) + sizeof(CE) + 1);
            
            /* Calculate and allign absolute root entry length */
            rr_file_list->ISO9660_len = rock_ridge_header_size + (DIR_RECORD_LEN + 1) + sizeof(SP) + sizeof(CE);
            (rr_file_list->ISO9660_len % 2 == 0) ? rr_file_list->ISO9660_len : rr_file_list->ISO9660_len++;
            
            /* CE entry should always exist for absolute root */
            rr_file_list->CE_LBA = 1;
            
            rr_file_list->full_len = rr_file_list->ISO9660_len + sizeof(ER);
         }
         
         /* Calculate how much blocks will parent directory occupy */
         while(tmp_file_list->next != NULL) {
            
            /* Skip absolute root direcstory */
            if (strncmp(tmp_file_list->name_path, ".", 2) == 0) {
               tmp_file_list = tmp_file_list->next;
               continue;
            }
            else if (tmp_file_list->parent_id == rr_file_list->dir_id) {
               int rr_len = 0;         // Represent individual len of each record
               
               pad_len = do_pad(tmp_file_list->name_conv_len, PAD_EVEN);
               terminator_len = set_terminator_len(tmp_file_list->st_mode);
               
               /* Size of basic iso9660 directory record */
               dir_rec_len += DIR_RECORD_LEN + tmp_file_list->name_conv_len + terminator_len + pad_len;
               rr_len = DIR_RECORD_LEN + tmp_file_list->name_conv_len + terminator_len + pad_len;
               
               if (rock_ridge_on == TRUE) {
                  if ((init_fields & rrip_NM) != 0)
                     additional_bytes = tmp_file_list->name_short_len + 5;
                  
                  dir_rec_len += rock_ridge_header_size + additional_bytes;
                  rr_len += rock_ridge_header_size + additional_bytes;
                  
                  /* 
                   * Set flag for CE creation if entry is longer than 255 bytes.
                   * tmp_file_list->full_len is used only if CE record is needed,
                   * otherwise tmp_file_list->ISO9660 is sufficient.
                   */
                  if (rr_len >= 0xFF) {
                     tmp_file_list->CE_LBA = 1;
                     tmp_file_list->ISO9660_len = DIR_RECORD_LEN + tmp_file_list->name_conv_len + terminator_len + pad_len + sizeof(CE);
                     
                     dir_rec_len -= rock_ridge_header_size + additional_bytes;
                     dir_rec_len += sizeof(CE);
                     
                     tmp_file_list->full_len = rr_len + sizeof(CE);
                     
                     /* 
                      * Create another NM entry if needed
                      * 250 is max len of payload for uint8 togeather with NM entry header.
                      * Without this NM entry len could overfow.
                      */
                     if (tmp_file_list->name_short_len >= 250)
                        tmp_file_list->full_len += 5;             // Count in another 5 bytes for NM entry header
                     
                     /* Allign to even bytes count in the end */
                     (tmp_file_list->full_len % 2 == 0) ? tmp_file_list->full_len : (tmp_file_list)->full_len++;
                     
                  }
                  else {
                     (rr_len % 2 == 0) ? rr_len : rr_len++;
                     tmp_file_list->ISO9660_len = rr_len;
                  }
                  
                  /* Allign to even bytes count in the end */
                  (dir_rec_len % 2 == 0) ? dir_rec_len : dir_rec_len++;
               }
               
               /* Allign entry at the end of the block */
               if (total_len + dir_rec_len <= counter * BLOCK_SIZE)
                  total_len += dir_rec_len;
               else {
                  total_len = counter * BLOCK_SIZE;
                  total_len += dir_rec_len;
                  counter++;
               }
               
               dir_rec_len = 0;
            }
            
            tmp_file_list = tmp_file_list->next;
         }
         
         if (total_len == 0)
            total_len = min_dir_len;
         
         rr_file_list->size = total_len;
         rr_file_list->blocks = blocks_count(total_len);
         
         /* 
          * Save largest continous directory block.
          * This will be used for memory allocation later in iso9660_directory_record().
          */
         if (rr_file_list->blocks > ISO_data->largest_cont_block)
            ISO_data->largest_cont_block = rr_file_list->blocks;
         
         rr_file_list->LBA = LBA;
         LBA += rr_file_list->blocks;
         
         /* 
          * Assign LBA to CE directory records.
          * Warning: This function increments LBA.
          */
         if (rr_file_list->CE_LBA == 1) {
            if ((rv = CE_assign_LBA(&CE_list, rr_file_list, &LBA)) != E_OK) {
               CEarr_destroy_list(&CE_list);
               return rv;
            }
         }
         
         ISO_data->dir_count++;
      }
      
      rr_file_list = rr_file_list->next;
   }
   CEarr_destroy_list(&CE_list);

   /* Initialize CE_list for non-directory records */
   if ((rv = CEarr_init_list(&CE_list, CE_LIST_PREALLOC)) != E_OK) {
      CEarr_destroy_list(&CE_list);
      return rv;
   }
   
   /* Assign CE LBAs to non-directory records */
   rr_file_list = file_list;
   while(rr_file_list->next != NULL) {
      if (rr_file_list->CE_LBA == 1) {
         
         /* 
          * Assign LBA to CE file records.
          * Warning: This function increments LBA.
          */
         if ((rv = CE_assign_LBA(&CE_list, rr_file_list, &LBA)) != E_OK) {
            CEarr_destroy_list(&CE_list);
            return rv;
         }
      }
      
      rr_file_list = rr_file_list->next;
   }
   CEarr_destroy_list(&CE_list);
   
   /* Assign first 2 file LBAs for BOOT.CAT and UEFI boot image (virtual image) */
   search_result = list_search(file_list, ISO_data->boot_cat_file);
   search_result->LBA = LBA;
   ISO_data->boot_cat_LBA = LBA;
   LBA += blocks_count(search_result->size);
   
   search_result = list_search(file_list, ISO_data->efi_boot_file_full);
   search_result->LBA = LBA;
   search_result->blocks = blocks_count(search_result->size);
   LBA += search_result->blocks;
   
   /* Assign LBA to files */
   rr_file_list = file_list;
   while(rr_file_list->next != NULL) {
      if (!S_ISDIR(rr_file_list->st_mode) && strncmp(rr_file_list->name_short, "BOOT.CAT", 8) \
      && strncmp(rr_file_list->name_path, ISO_data->efi_boot_file_full, strlen(ISO_data->efi_boot_file_full)) ) {

         rr_file_list->LBA = LBA;
         rr_file_list->blocks = blocks_count(rr_file_list->size);
         LBA = LBA + rr_file_list->blocks;
      }
      rr_file_list = rr_file_list->next;
   }
   
   ISO_data->LBA_last = LBA;
   
   return E_OK;
}

int iso9660_directory_record(struct file_list_t *file_list, FILE *dest, struct ISO_data_t *ISO_data) {
   void *directory_table = NULL;
   void *directory_table_start = NULL;
   void *rr_save_ptr = NULL;
   struct file_list_t *rr_file_list = file_list;
   struct file_list_t *tmp_file_list = NULL;
   int directory_table_size = BLOCK_SIZE * ISO_data->largest_cont_block;
   int rv = 0;
   int entry_len = 0;
   int bytes_written = 0;
   uint8_t rock_ridge_on = FALSE;
   int offset = 0;
   enum segment_list_t type = 0;
   
   if (option_on_off(ISO_data->options, OPT_R) == E_OK)
      rock_ridge_on = TRUE;
   
   if ((directory_table_start = (void *) malloc(directory_table_size)) == NULL) {
      printf("Error: iso9660_path_table(): Memory allocation failed\n");
      return E_MALLOC;
   }
   
   while(rr_file_list->next != NULL) {
      
      /* Skip files */
      if (!S_ISDIR(rr_file_list->st_mode)) {
         rr_file_list = rr_file_list->next;
         continue;
      }
      
      directory_table = directory_table_start;
      memset(directory_table_start, 0, directory_table_size);
      
      /* 
       * Entry for absolute root will be a bit different
       * Create ROOT record for CURRENT directory 
       */
      if (strncmp(rr_file_list->name_path, ".", 2) == 0)
         type = (rock_ridge_on == TRUE) ? RRIP_ABS_ROOT : ISO9660_ROOT;
      else
         type = (rock_ridge_on == TRUE) ? RRIP_ROOT : ISO9660_ROOT;
      
      bytes_written = construct_dir_record(rr_file_list, &directory_table, type);
      
      /*
       * Search whole file list from beginning
       * Search for all elements (dirs, files, ...), with matching parent ID
       */
      tmp_file_list = file_list;
      int CE_offset = 0;
      uint32_t CE_save_LBA = 0;
      while(tmp_file_list->next != NULL) {
         
         /*
          * Root entry in list was artificially created.
          * Following condition will skip it
          */
         if (tmp_file_list->parent_id == rr_file_list->dir_id && *tmp_file_list->name_short != 0 ) {
            
            /* Save pointer for later shift operations */
            rr_save_ptr = directory_table;
            
            type = (rock_ridge_on == TRUE) ? RRIP : ISO9660;
            
            entry_len = construct_dir_record(tmp_file_list, &directory_table, type);
            bytes_written += entry_len;
            
             /* Create CE record if needed */
            if (tmp_file_list->CE_LBA != 0) {
               void *rr_CE = CE + 4;
               void *CE_start = NULL;
               int basic_iso9660_len = 0;
               uint64_t CE_len = 0;
               uint64_t CE_LBA = 0;
               uint64_t CE_offset_LSB_MSB = 0;
               uint32_t rr_write = 0;
               uint8_t pad_len = 0;
               uint8_t terminator_len = 0;
               
               if (CE_save_LBA == 0)
                  CE_save_LBA = tmp_file_list->CE_LBA;
               
               if (CE_save_LBA != tmp_file_list->CE_LBA)
                  CE_offset = 0;
               
               terminator_len = set_terminator_len(tmp_file_list->st_mode);
               pad_len = do_pad(tmp_file_list->name_conv_len, PAD_EVEN);                                       // Dir record must start on EVEN byte
               basic_iso9660_len = DIR_RECORD_LEN + tmp_file_list->name_conv_len + pad_len + terminator_len;   // Length of basic dir record (without SUSP or RRIP)
               CE_LBA = get_int32_LSB_MSB(tmp_file_list->CE_LBA);
               CE_len = get_int32_LSB_MSB(entry_len - basic_iso9660_len);
               CE_offset_LSB_MSB = get_int32_LSB_MSB(CE_offset);
               
               fseek(dest, (tmp_file_list->CE_LBA * BLOCK_SIZE) + CE_offset, SEEK_SET);
               fwrite(rr_save_ptr + basic_iso9660_len, 1, entry_len - basic_iso9660_len, dest);                // Write everything after standard dir record
               
               memset(rr_CE, 0, BLOCK_SIZE - 4);
               iso9660_cp2heap(&rr_CE, &CE_LBA, sizeof(CE_LBA), &rr_write);                                    // rr_write is here only to ensure move of pointer
               iso9660_cp2heap(&rr_CE, &CE_offset_LSB_MSB, sizeof(CE_offset_LSB_MSB), &rr_write);
               iso9660_cp2heap(&rr_CE, &CE_len, sizeof(CE_len), &rr_write);
               
               /* Modify directory_table entry */
               CE_start = rr_save_ptr + basic_iso9660_len;
               iso9660_cp2heap(&CE_start, CE, sizeof(CE), &rr_write);
               iso9660_cp2heap(&rr_save_ptr, &(tmp_file_list)->ISO9660_len, sizeof(uint8_t), NULL);
               
               /* Clean bytes and correct counter value */
               memset(rr_save_ptr + tmp_file_list->ISO9660_len, 0, entry_len - tmp_file_list->ISO9660_len);
               directory_table -= entry_len;
               bytes_written -= entry_len;
               directory_table += tmp_file_list->ISO9660_len;
               bytes_written += tmp_file_list->ISO9660_len;
               
               tmp_file_list->CE_offset = CE_offset;
               CE_offset += (tmp_file_list->full_len - tmp_file_list->ISO9660_len);
               
               CE_save_LBA = tmp_file_list->CE_LBA;
            }
            
            /* Allign directory entry to BLOCK_SIZE */
            if (bytes_written > BLOCK_SIZE) {
               offset = entry_len + (BLOCK_SIZE - bytes_written);
               bytes_written = entry_len;
               
               /* Move bytes to new block */
               shift_mem(rr_save_ptr, offset, entry_len);
               directory_table += offset;
            }
         }
         
         tmp_file_list = tmp_file_list->next;
      }
      
      fseek(dest, rr_file_list->LBA * BLOCK_SIZE, SEEK_SET);
      fwrite(directory_table_start, 1, rr_file_list->blocks * BLOCK_SIZE, dest);
      
      rr_file_list = rr_file_list->next;
   }
   
   /* Write ER entry of ABS root directory */
   rr_file_list = file_list;
   fseek(dest, rr_file_list->CE_LBA * BLOCK_SIZE, SEEK_SET);
   fwrite(ER, 1, sizeof(ER), dest);
   
   /* Write files */
   rv = write_files(file_list, dest);

   free(directory_table_start);
   free(NM);
   
   return rv;
}

uint8_t do_pad(uint8_t len, enum pad_list_t type) {
   uint8_t pad_len = 0;
   
   if ((type == PAD_EVEN && len % 2 == 0) || (type == PAD_ODD && len % 2 != 0))
      pad_len = 1;
   
   return pad_len;
}

static uint32_t construct_dir_record(struct file_list_t *file_list, void **directory_table_output, enum segment_list_t type) {
   struct file_list_t *rr_file_list = file_list;
   struct tm *ts = NULL;
   void *rr_directory_table = *directory_table_output;
   void *rr_directory_table_start = rr_directory_table;
   void *rr_save_ptr = NULL;
   char *file_terminator = ";1";
   char mdate_time[7];
   char adate_time[7];
   char cdate_time[7];
   int terminator_len = 0;
   int rr_save_size = 0;
   int iso9660_original_size = 0;
   uint64_t data_len = 0;
   uint64_t LBA = 0;
   uint64_t mode = get_int32_LSB_MSB(rr_file_list->st_mode);
   uint64_t nlinks = get_int32_LSB_MSB(rr_file_list->st_nlink);
   uint64_t uid = get_int32_LSB_MSB(rr_file_list->st_uid);
   uint64_t gid = get_int32_LSB_MSB(rr_file_list->st_gid);
   uint64_t ino = get_int32_LSB_MSB(rr_file_list->st_ino);
   uint32_t directory_table_size = 0;
   uint32_t volume_seq_number = get_int16_LSB_MSB(one);
   uint8_t name_len = 0;
   uint8_t ext_attr_record = 0;
   uint8_t flags = 0;                                                // Will be adjusted in future, 0x02 indicates directory
   uint8_t pad_len = 0;
   
   memset(mdate_time, 0, sizeof(mdate_time));
   memset(adate_time, 0, sizeof(adate_time));
   memset(cdate_time, 0, sizeof(cdate_time));
   
   /* Assign modify date/time values */
   ts = localtime(&file_list->mtime);
   mdate_time[0] = ts->tm_year;
   mdate_time[1] = ts->tm_mon + 1;
   mdate_time[2] = ts->tm_mday;
   mdate_time[3] = ts->tm_hour;
   mdate_time[4] = ts->tm_min;
   mdate_time[5] = ts->tm_sec;
   mdate_time[6] = ts->tm_gmtoff / 60 / 15;
   
   /* Assign access date/time values */
   ts = localtime(&file_list->atime);
   adate_time[0] = ts->tm_year;
   adate_time[1] = ts->tm_mon + 1;
   adate_time[2] = ts->tm_mday;
   adate_time[3] = ts->tm_hour;
   adate_time[4] = ts->tm_min;
   adate_time[5] = ts->tm_sec;
   adate_time[6] = ts->tm_gmtoff / 60 / 15;
   
   /* Assign status change date/time values */
   ts = localtime(&file_list->ctime);
   cdate_time[0] = ts->tm_year;
   cdate_time[1] = ts->tm_mon + 1;
   cdate_time[2] = ts->tm_mday;
   cdate_time[3] = ts->tm_hour;
   cdate_time[4] = ts->tm_min;
   cdate_time[5] = ts->tm_sec;
   cdate_time[6] = ts->tm_gmtoff / 60 / 15;
   
   LBA = get_int32_LSB_MSB(rr_file_list->LBA);
   pad_len = do_pad(rr_file_list->name_conv_len, PAD_EVEN);
   
   if (S_ISDIR(rr_file_list->st_mode)) {
      flags = 0x02;
      terminator_len = 0;
      data_len = get_int32_LSB_MSB(rr_file_list->blocks * BLOCK_SIZE);
   }
   else {
      flags = 0x00;
      terminator_len = 2;
      data_len = get_int32_LSB_MSB(rr_file_list->size);
   }
   
   if (type == ISO9660_ROOT || type == ROOT_HEADER || type == RRIP_ROOT) {
      name_len = 1;                                                                                   // Lenght of roor dir name is always 1
      pad_len = 0;
   }
   else
      name_len = rr_file_list->name_conv_len + terminator_len;
   
   /* Create basic ISO9660 structure */
   iso9660_cp2heap(&rr_directory_table, &zero, sizeof(uint8_t), &directory_table_size);               // Lenght of whole entry will be added at the end
   iso9660_cp2heap(&rr_directory_table, &ext_attr_record, sizeof(uint8_t), &directory_table_size);    // Ext. attr. record
   iso9660_cp2heap(&rr_directory_table, &LBA, sizeof(uint64_t), &directory_table_size);               // LBA
   iso9660_cp2heap(&rr_directory_table, &data_len, sizeof(uint64_t), &directory_table_size);          // Size of file/dir
   iso9660_cp2heap(&rr_directory_table, mdate_time, sizeof(mdate_time), &directory_table_size);       // Recording date/time
   iso9660_cp2heap(&rr_directory_table, &flags, sizeof(uint8_t), &directory_table_size);              // Flags
   iso9660_cp2heap(&rr_directory_table, &zero, sizeof(uint8_t), &directory_table_size);               // Interleaving
   iso9660_cp2heap(&rr_directory_table, &zero, sizeof(uint8_t), &directory_table_size);               // Interleaving
   iso9660_cp2heap(&rr_directory_table, &volume_seq_number, sizeof(uint32_t), &directory_table_size); // Volume sequence number
   iso9660_cp2heap(&rr_directory_table, &name_len, sizeof(uint8_t), &directory_table_size);           // File name len
   
   /* Add file name */
   if (type == ISO9660_ROOT || type == ROOT_HEADER || type == RRIP_ROOT)
      iso9660_cp2heap(&rr_directory_table, &zero, name_len, &directory_table_size);
   else
      iso9660_cp2heap(&rr_directory_table, rr_file_list->name_conv, rr_file_list->name_conv_len, &directory_table_size);

   /* If entry is file, add file terminator */
   if (!S_ISDIR(rr_file_list->st_mode))
      iso9660_cp2heap(&rr_directory_table, file_terminator, terminator_len, &directory_table_size);
   
   /* Do padding of even len file names */
   if (pad_len == 1)
      iso9660_cp2heap(&rr_directory_table, &zero, pad_len, &directory_table_size);
   
   /* Save size of iso9660 basic entry */
   iso9660_original_size = directory_table_size;
   
   /* This one is called only during header creation */
   if (type == ROOT_HEADER) {
      iso9660_cp2heap(&rr_directory_table_start, &directory_table_size, sizeof(uint8_t), NULL);
   }
   /* Create root dir entry one more time, with name 0x01 ... */
   else if (type == ISO9660_ROOT) {
      iso9660_cp2heap(&rr_directory_table_start, &directory_table_size, sizeof(uint8_t), NULL);
      
      rr_save_ptr = rr_directory_table;
      rr_save_size = directory_table_size;
      
      iso9660_cp2heap(&rr_directory_table, rr_directory_table_start, iso9660_original_size - 1, &directory_table_size);
      iso9660_cp2heap(&rr_directory_table, &one, sizeof(uint8_t), &directory_table_size);
      
      rr_save_size = directory_table_size - rr_save_size;
      iso9660_cp2heap(&rr_save_ptr, &rr_save_size, sizeof(uint8_t), NULL);
   }
   /* Only add size of entry if RockRidge is not used */
   else if (type == ISO9660) {
      iso9660_cp2heap(&rr_directory_table_start, &directory_table_size, sizeof(uint8_t), NULL);
   }
   /* Add SUSP and RRIP data */
   else if (type == RRIP_ABS_ROOT || type == RRIP_ROOT) {
      RR[4] = 0;
      
      if ((init_fields & rrip_PX) != 0)
         RR[4] |= 1 << 0;                 // PX record in use
      if ((init_fields & rrip_TF) != 0)
         RR[4] |= 1 << 7;                 // TF record in use
      
      if (type == RRIP_ABS_ROOT) {
         void *rr_CE = CE + 4;
         uint64_t CE_size = get_int32_LSB_MSB(rr_file_list->CE_LBA);
         
         iso9660_cp2heap(&rr_CE, &CE_size, sizeof(uint64_t), NULL);
         rr_CE += 16;
         CE_size = get_int32_LSB_MSB(0xED);
         iso9660_cp2heap(&rr_CE, &CE_size, sizeof(uint64_t), NULL);
         
         iso9660_cp2heap(&rr_directory_table, SP, sizeof(SP), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, CE, sizeof(CE), &directory_table_size);
      }
      
      /* Write RR record*/
      if ((init_fields & rrip_RR) != 0)
         iso9660_cp2heap(&rr_directory_table, RR, sizeof(RR), &directory_table_size);
      
      /* Write PX record */
      if ((init_fields & rrip_PX) != 0) {
         iso9660_cp2heap(&rr_directory_table, PX, 4, &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &mode, sizeof(mode), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &nlinks, sizeof(nlinks), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &uid, sizeof(uid), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &gid, sizeof(gid), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &ino, sizeof(ino), &directory_table_size);
      }
      
      /* Write TF record */
      if ((init_fields & rrip_TF) != 0) {
         iso9660_cp2heap(&rr_directory_table, TF, 5, &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, mdate_time, sizeof(mdate_time), &directory_table_size);       // Modify date/time
         iso9660_cp2heap(&rr_directory_table, adate_time, sizeof(adate_time), &directory_table_size);       // Access date/time
         iso9660_cp2heap(&rr_directory_table, cdate_time, sizeof(cdate_time), &directory_table_size);       // status change date/time
      }
      
      /* End of record, do padding if needed */
      if (directory_table_size % 2 == 1)
         iso9660_cp2heap(&rr_directory_table, &zero, 1, &directory_table_size);
      
      /* Write size of first record */
      iso9660_cp2heap(&rr_directory_table_start, &directory_table_size, sizeof(uint8_t), NULL);
      
      rr_save_size = directory_table_size;
      rr_save_ptr = rr_directory_table;
      
      /* 
       * RECORD 2
       * All the circus one more time, this time with '1' 
       */   
      iso9660_cp2heap(&rr_directory_table, rr_directory_table_start, iso9660_original_size - 1, &directory_table_size);
      iso9660_cp2heap(&rr_directory_table, &one, sizeof(uint8_t), &directory_table_size);
      
      if ((init_fields & rrip_RR) != 0)
         iso9660_cp2heap(&rr_directory_table, RR, sizeof(RR), &directory_table_size);
      
      /* Write PX record */
      if ((init_fields & rrip_PX) != 0) {
         RR[4] |= 1 << 0;  // PX record in use
         iso9660_cp2heap(&rr_directory_table, PX, 4, &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &mode, sizeof(mode), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &nlinks, sizeof(nlinks), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &uid, sizeof(uid), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &gid, sizeof(gid), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &ino, sizeof(ino), &directory_table_size);
      }
      
      /* Write TF record */
      if ((init_fields & rrip_TF) != 0) {
         RR[4] |= 1 << 7;  // TF record in use
         iso9660_cp2heap(&rr_directory_table, TF, 5, &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, mdate_time, sizeof(mdate_time), &directory_table_size);       // Modify date/time
         iso9660_cp2heap(&rr_directory_table, adate_time, sizeof(adate_time), &directory_table_size);       // Access date/time
         iso9660_cp2heap(&rr_directory_table, cdate_time, sizeof(cdate_time), &directory_table_size);       // status change date/time
      }
      
      if (directory_table_size % 2 == 1)
         iso9660_cp2heap(&rr_directory_table, &zero, 1, &directory_table_size);
      
      /* Write size of second record */
      rr_save_size = directory_table_size - rr_save_size;
      iso9660_cp2heap(&rr_save_ptr, &rr_save_size, sizeof(uint8_t), NULL);
   }
   /* Add SUSP and RRIP data for regular entry */
   else if (type == RRIP) {
      uint8_t rest = 0;
      RR[4] = 0;
      
      if ((init_fields & rrip_PX) != 0)
         RR[4] |= 1 << 0;                 // PX record in use
      if ((init_fields & rrip_NM) != 0)
         RR[4] |= 1 << 3;                 // NM record in use
      if ((init_fields & rrip_TF) != 0)
         RR[4] |= 1 << 7;                 // TF record in use
      
      memset(NM + 5, 0, BLOCK_SIZE - 5);
      
      /* Prepare for another possible NM entry */
      if (rr_file_list->name_short_len >= 250) {
         rest = rr_file_list->name_short_len - 250;
         rr_file_list->name_short_len = 250;
      }
      
      NM[2] = 5 + rr_file_list->name_short_len;
      strncpy((char *)NM + 5, rr_file_list->name_short, rr_file_list->name_short_len);
      
      if ((init_fields & rrip_RR) != 0)
         iso9660_cp2heap(&rr_directory_table, RR, sizeof(RR), &directory_table_size);
      
      /* Set flag that NM entry will continue at next NM entry */
      if ((init_fields & rrip_NM) != 0) {
         if (rest != 0)
            NM[4] = 1;
         
         iso9660_cp2heap(&rr_directory_table, NM, rr_file_list->name_short_len + 5, &directory_table_size);
         
         /* Add second NM entry if needed */
         if (rest != 0) {
            memset(NM + 4, 0, BLOCK_SIZE - 4);
            NM[2] = 5 + rest;
            
            strncpy((char *)NM + 5, rr_file_list->name_short + rr_file_list->name_short_len, rest);
            
            iso9660_cp2heap(&rr_directory_table, NM, rest + 5, &directory_table_size);
         }
      }
         
      /* write PX record */
      if ((init_fields & rrip_PX) != 0) {
         iso9660_cp2heap(&rr_directory_table, PX, 4, &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &mode, sizeof(mode), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &nlinks, sizeof(nlinks), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &uid, sizeof(uid), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &gid, sizeof(gid), &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, &ino, sizeof(ino), &directory_table_size);
      }
      
      /* write TF record */
      if ((init_fields & rrip_TF) != 0) {
         iso9660_cp2heap(&rr_directory_table, TF, 5, &directory_table_size);
         iso9660_cp2heap(&rr_directory_table, mdate_time, sizeof(mdate_time), &directory_table_size);       // Modify date/time
         iso9660_cp2heap(&rr_directory_table, adate_time, sizeof(adate_time), &directory_table_size);       // Access date/time
         iso9660_cp2heap(&rr_directory_table, cdate_time, sizeof(cdate_time), &directory_table_size);       // status change date/time
      }
      
      if (directory_table_size % 2 != 0)
         iso9660_cp2heap(&rr_directory_table, &zero, 1, &directory_table_size);
         
      iso9660_cp2heap(&rr_directory_table_start, &directory_table_size, sizeof(uint8_t), NULL);
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

static int init_RRIP(void) {
   size_t rr_size = 0;
   int header_size = 0;
   char ER_text[] = "RRIP_1991ATHE ROCK RIDGE INTERCHANGE PROTOCOL PROVIDES SUPPORT FOR POSIX FILE SYSTEM \
SEMANTICSPLEASE CONTACT DISC PUBLISHER FOR SPECIFICATION SOURCE.  SEE PUBLISHER IDENTIFIER IN PRIMARY \
VOLUME DESCRIPTOR FOR CONTACT INFORMATION.";
   
   rr_size = sizeof(SP);
   memset(SP, 0, rr_size);
   SP[0] = 'S';
   SP[1] = 'P'; 
   SP[2] = rr_size;
   SP[3] = 1;           // SUSP Version
   SP[4] = 0xBE;        // Check byte
   SP[5] = 0xEF;        // Check byte
   SP[6] = 0;           // Len skip
   
   rr_size = sizeof(CE);
   memset(CE, 0, rr_size);
   CE[0] = 'C';
   CE[1] = 'E';
   CE[2] = rr_size;
   CE[3] = 1;           // SUSP Version
   
   rr_size = sizeof(ER);
   memset(ER, 0, rr_size);
   ER[0] = 'E';
   ER[1] = 'R';
   ER[2] = rr_size;     // Size
   ER[3] = 1;           // SUSP Version
   ER[4] = 0x0A;        // Len ID
   ER[5] = 0x54;        // Len descriptor
   ER[6] = 0x87;        // Len source
   ER[7] = 0x01;        // Extension version
   
   strcat((char *)ER, ER_text);
   
   if ((init_fields & rrip_RR) != 0) {
      rr_size = sizeof(RR);
      memset(RR, 0, rr_size);
      RR[0] = 'R';
      RR[1] = 'R';
      RR[2] = rr_size;
      RR[3] = 1;
      RR[4] = 0;
      header_size += rr_size;
   }
   
   if ((init_fields & rrip_PX) != 0) {
      rr_size = sizeof(PX);
      memset(PX, 0, rr_size);
      PX[0] = 'P';
      PX[1] = 'X';
      PX[2] = rr_size;
      PX[3] = 1;
      header_size += rr_size;
   }
   
   if ((init_fields & rrip_TF) != 0) {
      rr_size = sizeof(TF);
      memset(TF, 0, rr_size);
      TF[0] = 'T';
      TF[1] = 'F';
      TF[2] = rr_size;
      TF[3] = 1;
      TF[4] = 0x0E;                       // Record MODIFY, ACCESS and CREATION
      header_size += rr_size;
   }
   
   if ((init_fields & rrip_NM) != 0) {
      rr_size = BLOCK_SIZE;
      NM = (unsigned char*) malloc(rr_size);
      memset(NM, 0, rr_size);
      NM[0] = 'N';
      NM[1] = 'M';
      NM[2] = 0;
      NM[3] = 1;
      NM[4] = 0;
   }
   
   return header_size;
}

static void shift_mem(void *var, int offset, int ammount) {
   memmove(var + offset, var, ammount);
   memset(var, 0, offset);
}

static uint8_t set_terminator_len(mode_t mode) {
   return (S_ISDIR(mode)) ? 0 : 2;
}

