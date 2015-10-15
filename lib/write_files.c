/*
 * write_files.c
 * 
 * Version:       0.0.3
 * 
 * Release date:  10.10.2015
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

#include "write_files.h"

int write_files(struct file_list_t *file_list, FILE *dest) {
   struct file_list_t *rr_file_list = file_list;
   FILE *source_file = NULL;
   char buffer[BLOCK_SIZE];
   size_t bytes_read = 0;
   int mod = 0;
   memset(buffer, 0, sizeof(buffer));
   
   while(rr_file_list->next != NULL) {
      /*
       * Skip directories
       */
      if (S_ISDIR(rr_file_list->st_mode)) {
         rr_file_list = rr_file_list->next;
         continue;
      }
      
      fseek(dest, rr_file_list->LBA * BLOCK_SIZE, SEEK_SET);
      
      source_file = fopen(rr_file_list->name_path, "r");
      while(feof(source_file) == 0) {
         bytes_read = fread(buffer, 1, sizeof(buffer), source_file);
         if (fwrite(buffer, 1, bytes_read, dest) != bytes_read) {
            perror("Error: write_files()");
            fclose(source_file);
            return E_IO;
         }
      }
      
      memset(buffer, 0, sizeof(buffer));
      /*
       * Pad rest of the block
       */
      if ((mod = rr_file_list->size % BLOCK_SIZE) != 0) {
         if (fwrite(buffer, 1, BLOCK_SIZE - mod, dest) != BLOCK_SIZE - mod) {
            perror("Error: pad()");
            return E_IO;
         }
      }
      
      /*
       * If file is zero size write BLOCK_SIZE of zeros
       */
      if (rr_file_list->size == 0)
         if (fwrite(buffer, 1, BLOCK_SIZE, dest) != BLOCK_SIZE) {
            perror("Error: pad()");
            return E_IO;
         }
      
      fflush(dest);
      
      fclose(source_file);
      
      rr_file_list = rr_file_list->next;
   }
   
   return E_OK;
}
