/*
 * ebiso.c
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

#include "ebiso.h"

int main(int argc, char *argv[]) {
   struct file_list_t *list = (struct file_list_t*) malloc(sizeof(struct file_list_t));
   struct file_list_t *rr_list = list;
   struct ISO_data_t ISO_data;
   FILE *fp = NULL;
   FILE *boot_catalog = NULL;
   void *boot_descriptor = (void *) malloc(BLOCK_SIZE);
   void *header = NULL;
   void *terminator = NULL;
   void *path_table_LSB = NULL;
   void *path_table_MSB = NULL;
   int option = 0;
   int e_flag = 0;
   int o_flag = 0;
   int rv = 0;
   
   memset(list, 0, sizeof(struct file_list_t));
   memset(boot_descriptor, 0, BLOCK_SIZE);
   memset(&ISO_data, 0, sizeof(struct ISO_data_t));
   
   static struct option longOptions[] = {
      {"efi-boot", required_argument, 0, 'e'},
      {"output", required_argument, 0, 'o'},
      {"rock-ridge", no_argument, 0, 'R'},
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
      {0, 0, 0, 0}
   };
   
   while ((option = getopt_long(argc, argv, "e:ho:Rv", longOptions, NULL)) != -1) {
      switch(option) {
         case 'h':
            help_msg(MSG_USAGE);
            goto cleanup;
         break;
         case 'v':
            help_msg(MSG_VERSION);
            goto cleanup;
         break;
         case 'e':
            if (*optarg == '-') {
               printf("Error: %s is invalid argument for option -e\n", optarg);
               rv = E_BADSYNTAX;
               err_msg(rv);
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
               rv = E_BADSYNTAX;
               err_msg(rv);
               goto cleanup;
            }
            else {
               strncpy(ISO_data.iso_file, optarg, sizeof(ISO_data.iso_file));
               o_flag++;
               ISO_data.options |= (1 << OPT_o);
            }
         break;
         case 'R':
            if ((rv = set_option(&ISO_data.options, OPT_R)) != E_OK) {
               err_msg(rv);
               goto cleanup;
            }
         break;
         default:
            rv = E_BADSYNTAX;
            goto cleanup;
         break;
      }
   }
   
   /* First syntax check */
   if (e_flag != 1 || o_flag != 1 || argc < 6) {
      rv = E_BADSYNTAX;
      err_msg(rv);
      goto cleanup;
   }

   /* Working dir will be the last argument */
   strncpy(ISO_data.work_dir, argv[argc-1], MAX_DIR_STR_LEN);
   
   /* Yes, BOOT.CAT file will stay in (/) root of ISO file */
   snprintf(ISO_data.boot_cat_file, MAX_DIR_STR_LEN, "%s/%s", ISO_data.work_dir, "BOOT.CAT");
   snprintf(ISO_data.efi_boot_file_full, MAX_DIR_STR_LEN, "%s/%s", ISO_data.work_dir, ISO_data.efi_boot_file);
   
   /* Check if we can read working dir & add some stat info for ABS root directory */
   if ((rv = check_availability(ISO_data.work_dir, TYPE_DIR, MODE_READ, list)) != 0)
      goto cleanup;
   
   /* Fill some additional info about ABS root directory */
   strncpy(list->name_path, ".", 1);
   list->name_short_len = 1;
   list->name_conv_len = 1;
   list->dir_id = 1;
   list->parent_id = 1;
   list->next = (struct file_list_t*) malloc(sizeof(struct file_list_t));
   ISO_data.dir_count = 1;
   rr_list = list->next;
   memset(rr_list, 0, sizeof(struct file_list_t));
   
   /* Check if we can read UEFI boot image */
   if ((rv = check_availability(ISO_data.efi_boot_file_full, TYPE_FILE, MODE_READ, NULL)) != 0)
      goto cleanup;
   
   /*
    * Check if destination directory is writeable.
    * This will as well truncate existing file (if any) to zero
    */
   if ((fp = fopen(ISO_data.iso_file, "w")) == NULL) {
      printf("Error: Failed to create output file [%s]: %s\n", ISO_data.iso_file, strerror(errno));
      goto cleanup;
   }
   else
      fclose(fp);
   
   /* Delete dest iso file (if any) */
   unlink(ISO_data.iso_file);
   
   /* Create boot calalog (only as placeholder) */
   if ((boot_catalog = fopen(ISO_data.boot_cat_file, "w")) != NULL) {
      fwrite(boot_descriptor, 1, BLOCK_SIZE, boot_catalog);
      fclose(boot_catalog);
   }
   else {
      printf("Error: Failed to open [%s] for writing: %s\n", ISO_data.work_dir, strerror(errno));
      rv = E_WRFAIL;
      goto cleanup;
   }
   
   /* Create initial file structure */
   if ((rv = list_create(ISO_data.work_dir, &rr_list, &ISO_data)) != E_OK)
      goto cleanup;
   
   /* Remove possible duplicates */
   filename_rename_duplicates(list);
   
   /* Calculate path table offset, so LBAs can be offsetted in future */
   ISO_data.path_table_offset = get_path_table_offset(list);
   
   /* Assign future LBAs to created file list */
   if ((rv = iso9660_assign_LBA(list, &ISO_data)) != E_OK)
      goto cleanup;
      
   /* Write data to boot catalog */
   if ((rv = et_boot_catalog(ISO_data)) != E_OK)
      goto cleanup;
   
   /*
    * Create path table and assamble header
    * ISO_data in iso9660_path_table() is used to store path table size
    */
   if ((rv = iso9660_path_table(list, &path_table_LSB, LSB, &ISO_data)) != E_OK)
      goto cleanup;
   else {
      iso9660_path_table(list, &path_table_MSB, MSB, NULL);
      iso9660_header(&header, *list, ISO_data);
   }
   
   /* Assamble terminator */
   iso9660_terminator(&terminator);
   
   /* Assamble boot descriptor */
   et_boot_record_descr(&boot_descriptor, ISO_data);
   
   /* Write meta data to file */
   fp = fopen(ISO_data.iso_file, "w");
   fseek(fp, 0x8000, SEEK_SET);                                                     // Starting block in file
   fwrite(header, BLOCK_SIZE, 1, fp);                                               // 0x8000 - LBA 0x10 - Static
   fwrite(boot_descriptor, BLOCK_SIZE, 1, fp);                                      // 0x8800 - LBA 0x11 - Static
   fwrite(terminator, BLOCK_SIZE, 1, fp);                                           // 0x9000 - LBA 0x12 - Static
   fseek(fp, BLOCK_SIZE, SEEK_CUR);                                                 // 0x9800 - LBA 0x13 - Static
   
   fwrite(path_table_LSB, (ISO_data.path_table_offset + 1) * BLOCK_SIZE, 1, fp);    // (0xA000 - LBA 0x14) - Dynamic
   fseek(fp, BLOCK_SIZE, SEEK_CUR);                                                 // (0xA800 - LBA 0x15)
   fwrite(path_table_MSB, (ISO_data.path_table_offset + 1) * BLOCK_SIZE, 1, fp);    // (0xB000 - LBA 0x16)
   fseek(fp, BLOCK_SIZE, SEEK_CUR);                                                 // (0xB800 - LBA 0x17)
   
   /* If write of files failed, get rid of output iso file */
   if ((rv = iso9660_directory_record(list, fp, &ISO_data)) != E_OK)                // 0xC000
      unlink(ISO_data.iso_file);
   else {
      fseek(fp, BLOCK_SIZE - 1, SEEK_CUR);
      fwrite(&zero, 1, 1, fp);                                                      // Write one empty block at the end
   }
   
   fclose(fp);
   
#if (DEBUG == 1)
   printf("DEBUG: %s(): Number of directories: [%d]\n", __func__, ISO_data.dir_count);
   int i = 0;
   printf("DEBUG: %s(): MAIN STRUCTURE DUMP:\n", __func__);
   printf("%-5s %-4s %-55s %-11s %-3s %-9s %-5s %-7s %-12s %-5s %-11s %-6s %-8s %-7s\n", \
      "Level", "PID", "Name", "Size", "ID", "LBA", "Flag", "Blocks", "conf_name", \
      "Len", "ISO9660_len", "CE_LBA", "Full_len", "CE_off");
   for (i = 0; i <= 128; i++)
      disp_level(list, i);
#endif

cleanup:
   
   list_clean(list);
   if (terminator != NULL)
      free(terminator);
   if (path_table_LSB != NULL)
      free(path_table_LSB);
   if (path_table_MSB != NULL)
      free(path_table_MSB);
   if (header != NULL)
      free(header);
   if (boot_descriptor != NULL)
      free(boot_descriptor);
   
   return rv;
}

#if (DEBUG == 1)
static void disp_level(struct file_list_t *list_to_display, int level) {
   char flag = 0;
   struct tm *ts;
   char buff[80];
   
   while(list_to_display->next != NULL) {
      if (list_to_display->level == level) {
         if (S_ISDIR(list_to_display->st_mode))
            flag = 'D';
         else
            flag = 'F';
         
         if (S_ISLNK(list_to_display->st_mode))
            flag = 'L';
         
         ts = localtime(&list_to_display->mtime);
         strftime(buff, sizeof(buff), "%a %Y-%m-%d %H:%M:%S %Z", ts);
         
         printf("%-5d %-4d %-55s 0x%-9x %-3d 0x%-7x %-5c %-7d %-12s %-5d 0x%-9x 0x%-4x 0x%-6x 0x%-5x\n", \
            level, list_to_display->parent_id, list_to_display->name_path, list_to_display->size, \
            list_to_display->dir_id, list_to_display->LBA, flag, list_to_display->blocks, \
            list_to_display->name_conv, list_to_display->name_conv_len, list_to_display->ISO9660_len, \
            list_to_display->CE_LBA, list_to_display->full_len, list_to_display->CE_offset);
      }
      
      list_to_display = list_to_display->next;
   }
}
#endif

static int set_option(uint32_t *opt2set, enum opt_l option) {
   if ((*opt2set & (1 << option)) != 0)
      return E_BADSYNTAX;
   else
      *opt2set |= (1 << option);
   
   return E_OK;
}

int option_on_off(uint32_t option2check, enum opt_l option) {
   if ((option2check & (1 << option)) != 0)
      return E_OK;
   else
      return E_NOTSET;
}

static int check_availability(char *filename, enum check_type_l type, enum check_mode_l mode, struct file_list_t *file_list) {
   struct stat rr_stat;
   int rv = E_OK;
   
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
   
   if (file_list != NULL) {
      file_list->size = rr_stat.st_size;
      file_list->st_mode = rr_stat.st_mode;
      file_list->st_uid = rr_stat.st_uid;
      file_list->st_gid = rr_stat.st_gid;
      file_list->st_nlink = rr_stat.st_nlink;
      file_list->st_ino = rr_stat.st_ino;
      file_list->mtime = rr_stat.st_mtime;
      file_list->atime = rr_stat.st_atime;
      file_list->ctime = rr_stat.st_ctime;
   }
   
   
   return rv;
}

static void help_msg(enum msg_l id) {
   if (id == MSG_USAGE) {
   printf("\
Usage: \n\
  %s -e FILE -o FILE [-R] WORK_DIR\n\
  %s -h\n\
  %s -v\n\
\n\
Create UEFI bootable ISO image.\n\
\n\
Arguments:\n\
  -e, --efi-boot=FILE                  Bootable image location.\n\
                                       WARNING: Path must be relative to WORK_DIR!\n\
  -o, --output=FILE                    Output file location. File will be overwritten, if exists.\n\
  -R, --rock-ridge                     Use Rock Ridge Interchange Protocol (RRIP).\n\
  -h, --help                           This screen.\n\
  -v, --version                        Display version.\n", PROGNAME, PROGNAME, PROGNAME);
   }
   else if (id == MSG_VERSION)
      printf("%s\n", EBISO_VERSION);
}

static void err_msg(enum errors_l error) {
   if (error == E_BADSYNTAX) {
      printf("%s: Bad syntax\n", PROGNAME);
      printf("Try '%s --help' for more information.\n", PROGNAME);
   }
}

static uint32_t get_path_table_offset(struct file_list_t *file_list) {
   uint8_t pad_len = 0;
   uint32_t path_table_size = 0;
   
   while(file_list->next != NULL ) {
      if (S_ISDIR(file_list->st_mode)) {
         pad_len = do_pad(file_list->name_conv_len, PAD_ODD);
         
         path_table_size += PT_RECORD_LEN + pad_len;
      }
      
      file_list = file_list->next;
   }
   
   return path_table_size / (BLOCK_SIZE + 1);
   
}
