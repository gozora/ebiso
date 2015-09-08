/*
 * ebiso.c
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

#include "ebiso.h"
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
   struct ISO_data_t ISO_data;
   FILE *fp = NULL;
   FILE *boot_catalog = NULL;
   struct file_list_t *list = (struct file_list_t*) malloc(sizeof(struct file_list_t));
   struct file_list_t *rr_list = list;
   void *header = NULL;
   void *terminator = NULL;
   void *path_table = NULL;
   void *boot_descriptor = (void *) malloc(BLOCK_SIZE);
   uint32_t path_table_size = 0;
   char iso_file[MAX_DIR_STR_LEN];
   int option = 0;
   int e_flag = 0;
   int o_flag = 0;
   int rv = 0;
   
   memset(list, 0, sizeof(struct file_list_t));
   memset(boot_descriptor, 0, BLOCK_SIZE);
   memset(&ISO_data, 0, sizeof(struct ISO_data_t));
   memset(iso_file, 0, MAX_DIR_STR_LEN);
   
   /*
    * Root directory
    */
   strncpy(list->name_path, ".", 1);
   list->name_short_len = 1;
   list->size = 4096;
   list->st_mode = 16877;
   list->dir_id = 1;
   list->next = (struct file_list_t*) malloc(sizeof(struct file_list_t));
   list->parent_id = 1;
   rr_list = list->next;
   memset(rr_list, 0, sizeof(struct file_list_t));
   
   static struct option longOptions[] = {
      {"efi-boot", required_argument, 0, 'e'},
      {"output", required_argument, 0, 'o'},
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
      {0, 0, 0, 0}
   };
   
   while ((option = getopt_long(argc, argv, "e:ho:v", longOptions, NULL)) != -1) {
      switch(option) {
         case 'h':
            msg(MSG_USAGE);
            goto cleanup;
         break;
         case 'v':
            msg(MSG_VERSION);
            goto cleanup;
         break;
         case 'e':
            if (*optarg == '-') {
               printf("Error: %s is invalid argument for option -e\n", optarg);
               msg(MSG_SYNTAX);
               rv = E_BADSYNTAX;
               goto cleanup;
            }
            else {
               strncpy(ISO_data.efi_boot_file, optarg, sizeof(ISO_data.efi_boot_file));
               e_flag++;
            }
         break;
         case 'o':
            if (*optarg == '-') {
               printf("Error: %s is invalid argument for option -o\n", optarg);
               msg(MSG_SYNTAX);
               rv = E_BADSYNTAX;
               goto cleanup;
            }
            else {
               strncpy(iso_file, optarg, sizeof(iso_file));
               o_flag++;
            }
         break;
         default:
            rv = E_BADSYNTAX;
            goto cleanup;
         break;
      }
   }
   
   /*
    * First syntax check
    */
   if (e_flag != 1 || o_flag != 1 || argc != 6) {
      msg(MSG_SYNTAX);
      rv = E_BADSYNTAX;
      goto cleanup;
   }

   /* Working dir will be the last argument */
   strncpy(ISO_data.work_dir, argv[argc-1], MAX_DIR_STR_LEN);
   snprintf(ISO_data.boot_cat_file, MAX_DIR_STR_LEN, "%s/%s", ISO_data.work_dir, "BOOT.CAT");
   snprintf(ISO_data.efi_boot_file_full, MAX_DIR_STR_LEN, "%s/%s", ISO_data.work_dir, ISO_data.efi_boot_file);
   
   /*
    * Check if we can read working dir
    */
   if ((rv = check_availability(ISO_data.work_dir, TYPE_DIR, MODE_READ)) != 0)
      goto cleanup;
   
   /*
    * Check if we can read efiboot.img 
    */
   if ((rv = check_availability(ISO_data.efi_boot_file_full, TYPE_FILE, MODE_READ)) != 0)
      goto cleanup;
   
   /*
    * Try to create dest iso file
    * This will truncate existing file (if any)
    */
   if ((fp = fopen(iso_file, "w")) == NULL) {
      printf("Error: Failed to create output file [%s]: %s\n", iso_file, strerror(errno));
      goto cleanup;
   }
   else
      fclose(fp);
   
   /*
    * Delete dest iso file (if any)
    */
   unlink(iso_file);
   
   /* 
    * Create boot calalog (only as placeholder)
    */
   if ((boot_catalog = fopen(ISO_data.boot_cat_file, "w")) != NULL) {
      fwrite(boot_descriptor, 1, BLOCK_SIZE, boot_catalog);
      fclose(boot_catalog);
   }
   else {
      printf("Error: Failed to open [%s] for writing: %s\n", ISO_data.work_dir, strerror(errno));
      rv = E_WRFAIL;
      goto cleanup;
   }
   
   /*
    * Create initial file structure either from current dir (no argument was given)
    * or from user given argument
    */
   if ((rv = fill(ISO_data.work_dir, &rr_list)) != 0)
      goto cleanup;
   
   /*
    * Assign future LBAs
    */
   if (iso9660_assign_LBA(list, &ISO_data) == -1) {
      rv = E_NOTFOUND;
      goto cleanup;
   }
      
   /*
    * Write data to boot catalog
    */
   et_boot_catalog(ISO_data, ISO_data.work_dir);
   
   /*
    * Create path table and assamble header
    */
   path_table_size = iso9660_path_table(list, &path_table);
   iso9660_header(path_table_size, &header, *list, ISO_data.LBA_last);
   
   /*
    * Assamble terminator
    */
   iso9660_terminator(&terminator);
   
   /*
    * Assamble boot descriptor
    */
   et_boot_record_descr(&boot_descriptor, ISO_data);
   
   /*
    * Write all to file
    */
   fp = fopen(iso_file, "w");
   fseek(fp, 0x8000, SEEK_SET);                                // Starting block in file
   fwrite(header, BLOCK_SIZE, 1, fp);                          // 0x8000
   fwrite(boot_descriptor, BLOCK_SIZE, 1, fp);                 // 0x8800
   fwrite(terminator, BLOCK_SIZE, 1, fp);                      // 0x9000
   fseek(fp, BLOCK_SIZE, SEEK_CUR);                            // 0x9800
   
   fwrite(path_table, BLOCK_SIZE, 1, fp);                      // 0xA000 - needs to be adjusted for larger path tables ...
   fseek(fp, BLOCK_SIZE, SEEK_CUR);                            // 0xA800 - one block empty
   fseek(fp, BLOCK_SIZE, SEEK_CUR);                            // 0xB000 - BE path table will be here
   fseek(fp, BLOCK_SIZE, SEEK_CUR);                            // 0xB800 - one block empty
   
   /*
    * Write of files failed, get rid of iso_file               
    */
   if ((rv = iso9660_directory(list, fp)) != OK)               // 0xC000
      unlink(iso_file);

   fclose(fp);
   
#ifdef DEBUG
   int i = 0;
   switch (DEBUG) {
      case 1:
         printf("%-5s %-4s %-40s %-11s %-3s %-7s %-5s\n", "Level", "PID", "Name", "Size", "ID", "LBA", "Flag");
         for (i = 0; i <= 8; i++)
            disp_level(list, i);
      break;
   }
#endif

cleanup:
   
   clean_list(list);
   free(terminator);
   free(path_table);
   free(header);
   free(boot_descriptor);
   
   return rv;
}

#ifdef DEBUG
static void disp_level(struct file_list_t *list_to_display, int level) {
   char flag = 0;
   
   while(list_to_display->next != NULL) {
      if (list_to_display->level == level) {
         if (S_ISDIR(list_to_display->st_mode))
            flag = 'D';
         else
            flag = 'F';
      
         printf("%-5d %-4d %-40s %-11d %-3d %-7d %-5c\n", level, list_to_display->parent_id, list_to_display->name_path, list_to_display->size, \
         list_to_display->dir_id, list_to_display->LBA, flag);
      }
      
      list_to_display = list_to_display->next;
   }
}
#endif

static void clean_list(struct file_list_t *list_to_clean) {
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

static int check_availability(char *filename, enum check_type_l type, enum check_mode_l mode) {
   struct stat rr_stat;
   int rv = 0;
   
   memset(&rr_stat, 0, sizeof(struct stat));
   
   if (stat(filename, &rr_stat) != 0) {
      printf("Error: check_availability(): Failed to stat [%s]: %s\n", filename, strerror(errno));
      rv = E_STATFAIL;
   }
   
   if (mode == MODE_READ && rv == 0) {
      if (access(filename, R_OK) != 0) {
         printf("Error: check_availability():  Failed to read [%s]: %s\n", filename, strerror(errno));
         rv = E_READFAIL;
      }
   }
   else if (mode == MODE_WRITE && rv == 0) {
      if (access(filename, W_OK) != 0) {
         printf("Error: check_availability():  Failed to write [%s]: %s\n", filename, strerror(errno));
         rv = E_WRFAIL;
      }
   }
   
   if (type == TYPE_FILE && rv == 0) {
      if (!S_ISREG(rr_stat.st_mode)) {
         printf("Error: check_availability():  Not a regular file [%s]\n", filename);
         rv = E_NOTREG;
      }
   }
   
   if (type == TYPE_DIR && rv == 0) {
      if (!S_ISDIR(rr_stat.st_mode)) {
         printf("Error: check_availability():  Not a directory [%s]\n", filename);
         rv = E_NOTDIR;
      }
   }
   
   return rv;
}

static void msg(enum msg_l id) {
   if (id == MSG_USAGE) {
   printf("\
Usage: \n\
  %s -e FILE -o FILE WORK_DIR\n\
  %s -h\n\
  %s -v\n\
\n\
Create UEFI bootable ISO image.\n\
\n\
Arguments:\n\
  -e, --efi-boot=FILE                  bootable image location\n\
                                       WARNING: must be relative to WORK_DIR!\n\
  -o, --output=FILE                    output file location,\n\
                                       WARNING: file will be overwritten, if exists!\n\
  -h, --help                           this screen\n\
  -v, --version                        display version\n", PROGNAME, PROGNAME, PROGNAME);
   }
   else if (id == MSG_VERSION)
      printf("%s\n", VERSION);
   else if (id == MSG_SYNTAX) {
      printf("%s: Bad syntax\n", PROGNAME);
      printf("Try '%s --help' for more information.\n", PROGNAME);
   }
}
