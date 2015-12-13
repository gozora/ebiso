/*
 * list.c
 * 
 * Version:       0.2.1
 * 
 * Release date:  13.12.2015
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

int list_create(const char *dirname, struct file_list_t **flist, struct ISO_data_t *ISO_data) {
   FILE *read_test = NULL;
   DIR *cur_dir = NULL;
   struct dirent *dir_content = NULL;
   struct stat dir_cont_stat;
   char path[MAX_DIR_STR_LEN];
   static int parent_id = 1;
   static int dir_id = 1;
   static int level = 0;
   int rr_dir = 0;
   int path_len = 0;
   int rv = E_OK;
   bool_t rock_ridge_on = FALSE;
   
   if (option_on_off(ISO_data->options, OPT_R) == E_OK)
      rock_ridge_on = TRUE;
   
   /* Record/increase current directory level */
   level++;
   
   /* Fail if some (sub) directory is unreadable */
   if ( (cur_dir = opendir(dirname)) == NULL ) {
      printf("Error: list_create(): Opening directory [%s] failed: %s\n", dirname, strerror(errno));
      rv = E_READFAIL;
      goto cleanup;
   }
   
   /* Read whole directory content */
   while ( (dir_content = readdir(cur_dir)) != NULL && (rv == 0) ) {
      if ( (strcmp(dir_content->d_name, "..") == 0) || (strcmp(dir_content->d_name, ".") == 0) )
         continue;
      
      /* Stop if number of characters in path reaches the limit */
      if ( (path_len = snprintf(path, MAX_DIR_STR_LEN, "%s/%s", dirname, dir_content->d_name)) > MAX_DIR_STR_LEN ) {
         printf("Error: list_create(): Max path lenght limit[%d] reached [%s/%s]\n", MAX_DIR_STR_LEN, dirname, dir_content->d_name);
         rv = E_FILELIMIT;
      }
      
      /* Fill structure with directory data */
      else {
         strncpy((*flist)->name_path, path, path_len + 1);   // Copy trailing null
         strncpy((*flist)->name_short, dir_content->d_name, MAX_DIR_STR_LEN - 1);
         
         /* convert filename to 8.3 format */
         (*flist)->name_conv_len = filename_convert_name((*flist)->name_short, (*flist)->name_conv, CONV_ISO9660);
         
         lstat(path, &dir_cont_stat);
         
         /* Don't do read test on symlinks as they don't need to have any real valid content */
         if (S_ISLNK(dir_cont_stat.st_mode) && rock_ridge_on == TRUE) {
            memset((*flist)->name_path, 0, MAX_DIR_STR_LEN);
            if (readlink(path, (*flist)->name_path, MAX_DIR_STR_LEN - 1) == -1) {
               printf("Error: list_create(): Failed to read link [%s]: %s\n", path, strerror(errno));
               goto cleanup;
            }
         }
         /* Symlinks will be ignored if RRIP is not in use */
         else if (S_ISLNK(dir_cont_stat.st_mode) && rock_ridge_on == FALSE) {
            printf("Warning: list_create(): Symlink will be ignored: [%s]\n", path);
            memset(*flist, 0, sizeof(struct file_list_t));
            continue;
         }
         /* Do a read test on files and dirs */
         else {
            stat(path, &dir_cont_stat);
            if ((read_test = fopen(path, "r")) == NULL) {
               printf("Error: list_create(): Failed to open [%s]: %s\n", path, strerror(errno));
               rv = E_READFAIL;
               goto cleanup;
            }
            else
               fclose(read_test);
         }
         
         if (S_ISDIR(dir_cont_stat.st_mode))
            dir_id++;
         
         (*flist)->name_short_len = strlen((*flist)->name_short);
         (*flist)->size = dir_cont_stat.st_size;
         (*flist)->st_mode = dir_cont_stat.st_mode;
         (*flist)->st_nlink = dir_cont_stat.st_nlink;
         (*flist)->st_uid = dir_cont_stat.st_uid;
         (*flist)->st_gid = dir_cont_stat.st_gid;
         (*flist)->st_ino = dir_cont_stat.st_ino;
         (*flist)->mtime = dir_cont_stat.st_mtime;
         (*flist)->atime = dir_cont_stat.st_atime;
         (*flist)->ctime = dir_cont_stat.st_ctime;
         (*flist)->dir_id = dir_id;
         (*flist)->parent_id = parent_id;
         (*flist)->level = level;
         (*flist)->next = (struct file_list_t*) malloc(sizeof(struct file_list_t));
         (*flist) = (*flist)->next;
         
         memset(*flist, 0, sizeof(struct file_list_t));
      }
      
      /* Recursion to child directory */
      if ( !(dir_content->d_type ^ DT_DIR) && (rv == E_OK) ) {
         rr_dir = parent_id;
         parent_id = dir_id;
         
         rv = list_create(path, flist, ISO_data);
         
         parent_id = rr_dir;
         ISO_data->dir_count++;
      }
   }
   
cleanup:
   closedir(cur_dir);
   level--;
   
   return rv;
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

struct file_list_t *list_search_name(struct file_list_t *file_list, char *needle) {
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
