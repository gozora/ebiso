/*
 * list.c
 * 
 * Version:       0.0.3-alfa
 * 
 * Release date:  17.09.2015
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

#include "list.h"
#include <dirent.h>
#include <errno.h>

int list_create(const char *dirname, struct file_list_t **flist) {
   FILE *read_test = NULL;
   DIR *cur_dir = NULL;
   struct dirent *dir_content = NULL;
   struct stat dir_cont_stat;
   char path[MAX_DIR_STR_LEN];
   char tmp_conv[MAX_DIR_STR_LEN];
   static int parent_id = 1;
   static int dir_id = 1;
   static int level = 0;
   int rr_dir = 0;
   int path_len = 0;
   int rc = 0;
   
   memset(tmp_conv, 0, sizeof(tmp_conv));
   
   /* Record/increase current directory level */
   level++;
   
   /* Fail if some (sub) directory is unreadable */
   if ( (cur_dir = opendir(dirname)) == NULL ) {
      printf("Error: list_create(): Opening directory [%s] failed: %s\n", dirname, strerror(errno));
      rc = E_READFAIL;
      goto cleanup;
   }
   
   /* Read whole directory content */
   while ( (dir_content = readdir(cur_dir)) != NULL && (rc == 0) ) {
      if ( (strcmp(dir_content->d_name, "..") == 0) || (strcmp(dir_content->d_name, ".") == 0) )
         continue;
      
      /* Stop if number of characters in path reaches the limit */
      if ( (path_len = snprintf(path, MAX_DIR_STR_LEN, "%s/%s", dirname, dir_content->d_name)) > MAX_DIR_STR_LEN ) {
         printf("Error: list_create(): Max path lenght limit[%d] reached [%s/%s]\n", MAX_DIR_STR_LEN, dirname, dir_content->d_name);
         rc = E_FILELIMIT;
      }
      
      /* Fill structure with directory data */
      else {
         strncpy((*flist)->name_path, path, path_len + 1);   // Copy trailing null
         strncpy((*flist)->name_short, dir_content->d_name, MAX_DIR_STR_LEN - 1);
         
         /*
          * Temporary disabled, until implementation of RRIP is finished
          * Medium is currently not fully ISO compatible, but should be working anyhow
          */
         /* convert filename to 8.3 format */
         //(*flist)->name_conv_len = convert_name((*flist)->name_short, (*flist)->name_conv, CONV_ISO9660);
         (*flist)->name_conv_len = strlen((*flist)->name_short);
         strncpy((*flist)->name_conv, (*flist)->name_short, (*flist)->name_conv_len);
         
         
         if ((read_test = fopen(path, "r")) == NULL) {
            printf("Error: list_create(): Failed to open [%s]: %s\n", path, strerror(errno));
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
         (*flist)->mtime = dir_cont_stat.st_mtime;
         (*flist)->dir_id = dir_id;
         (*flist)->next = (struct file_list_t*) malloc(sizeof(struct file_list_t));
         (*flist)->parent_id = parent_id;
         (*flist)->level = level;
         (*flist) = (*flist)->next;
         
         memset(*flist, 0, sizeof(struct file_list_t));
         memset(tmp_conv, 0, sizeof(tmp_conv));
      }
      
      /* Recursion to child directory */
      if ( !(dir_content->d_type ^ DT_DIR) && (rc == 0) ) {
         rr_dir = parent_id;
         parent_id = dir_id;
         
         rc = list_create(path, flist);
         
         parent_id = rr_dir;
      }
   }
   
cleanup:
   closedir(cur_dir);
   level--;
   
   return rc;
}

void list_clean(struct file_list_t *list_to_clean) {
   struct file_list_t *curr = NULL;
   struct file_list_t *head = NULL;
   
   curr = list_to_clean->next;

   while (curr != NULL) {
      head = curr->next;
      free(curr);
      curr = head;
   }
   
   free(list_to_clean);
}

struct file_list_t *list_search(struct file_list_t *file_list, char *needle) {
   struct file_list_t *rv = NULL;
   size_t needle_len = strlen(needle);
   
   while(file_list->next != NULL) {
      if (S_ISDIR(file_list->st_mode)) {
         file_list = file_list->next;
         continue;
      }
      else {
         if (strncmp(file_list->name_path, needle, needle_len) == 0) {
            rv = file_list;
            break;
         }
         else
            file_list = file_list->next;
      }
   }
   
   return rv;
}

