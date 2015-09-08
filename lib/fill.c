/*
 * fill.c
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

#include "fill.h"
#include <dirent.h>
#include <errno.h>

int fill(const char *dirname, struct file_list_t **flist) {
   FILE *read_test = NULL;
   DIR *cur_dir = NULL;
   struct dirent *dir_content = NULL;
   char path[MAX_DIR_STR_LEN];
   int path_len = 0;
   int rc = 0;
   struct stat dir_cont_stat;
   static int parent_id = 1;
   static int dir_id = 1;
   static int level = 0;
   int rr_dir = 0;
   
   level++; // Record/increase current directory level
   
   /*
    * If user provided argument is not a directory, quit
    */
   if ( (cur_dir = opendir(dirname)) == NULL ) {
      printf("Error: Opening directory [%s] failed: %s\n", dirname, strerror(errno));
      rc = E_READFAIL;
      goto cleanup;
   }
   
   /*
    * Read whole directory content
    */
   while ( (dir_content = readdir(cur_dir)) != NULL && (rc == 0) ) {
      if ( (strcmp(dir_content->d_name, "..") == 0) || (strcmp(dir_content->d_name, ".") == 0) )
         continue;
      
      /*
       * Stop if number of characters in path reaches the limit
       */
      if ( (path_len = snprintf(path, MAX_DIR_STR_LEN, "%s/%s", dirname, dir_content->d_name)) > MAX_DIR_STR_LEN ) {
         printf("Error: Max path lenght limit[%d] reached [%s/%s]\n", MAX_DIR_STR_LEN, dirname, dir_content->d_name);
         rc = E_FILELIMIT;
      }
      
      /*
       * Fill structure with directory data
       */
      else {
         strncpy((*flist)->name_path, path, path_len + 1);   // Copy trailing null
         strncpy((*flist)->name_short, dir_content->d_name, MAX_DIR_STR_LEN - 1);
         
         if ((read_test = fopen(path, "r")) == NULL) {
            printf("Error: Failed to open [%s]: %s\n", path, strerror(errno));
            rc = E_READFAIL;
            goto cleanup;
         }
         else
            fclose(read_test);
         
         stat(path, &dir_cont_stat);
         
         if (S_ISDIR(dir_cont_stat.st_mode))
            dir_id++;
         
         (*flist)->name_short_len = strlen((*flist)->name_short);
         (*flist)->size = dir_cont_stat.st_size;
         (*flist)->st_mode = dir_cont_stat.st_mode;
         (*flist)->dir_id = dir_id;
         (*flist)->next = (struct file_list_t*) malloc(sizeof(struct file_list_t));
         (*flist)->parent_id = parent_id;
         (*flist)->level = level;
         (*flist) = (*flist)->next;
         
         memset(*flist, 0, sizeof(struct file_list_t));
      }
      
      /*
       * Recursion to child directory
       */
      if ( !(dir_content->d_type ^ DT_DIR) && (rc == 0) ) {
         rr_dir = parent_id;
         parent_id = dir_id;
         
         rc = fill(path, flist);
         
         parent_id = rr_dir;
      }
   }
   
cleanup:
   closedir(cur_dir);
   level--;
   
   return rc;
}


