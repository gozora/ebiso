/*
 * ebiso.h
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

#include "globals.h"
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#define PROGNAME "ebiso"
#define VERSION "0.0.2-alfa"

#define DEBUG 1
#ifdef DEBUG
static void disp_level(struct file_list_t *list_to_display, int level);
#endif

enum check_type_l {
   TYPE_FILE,
   TYPE_DIR
} check_type_l;

enum check_mode_l {
   MODE_READ,
   MODE_WRITE
} check_mode_l;

enum msg_l {
   MSG_USAGE,
   MSG_VERSION,
   MSG_SYNTAX
} msg_l;

extern int list_create(const char *dirname, struct file_list_t **flist);
extern void list_clean(struct file_list_t *list_to_clean);

extern uint32_t iso9660_header(void **header, struct file_list_t file_list, struct ISO_data_t ISO_data);
extern uint32_t iso9660_terminator(void **terminator);
extern int iso9660_path_table(struct file_list_t *file_list, void **path_table, enum endianity_l endianity, struct ISO_data_t *ISO_data);
extern int iso9660_assign_LBA(struct file_list_t *file_list, struct ISO_data_t *ISO_data);
extern int iso9660_directory(struct file_list_t *file_list, FILE *dest);

extern void et_boot_record_descr(void **boot_record_descriptor, struct ISO_data_t ISO_data);
extern int et_boot_catalog(struct ISO_data_t LBA_data);

static int check_availability(char *filename, enum check_type_l type, enum check_mode_l mode);
static void msg(enum msg_l id);
static uint32_t get_path_table_offset(struct file_list_t *file_list);
