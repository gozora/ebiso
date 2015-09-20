/*
 * filename.c
 * 
 * Version:       0.0.1-alfa
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

#include "filename.h"

void filename_rename_duplicates(struct file_list_t *list) {
   struct file_list_t *rr_list = list;
   struct file_list_t *rr2_list = list;
   int dup_counter = 0;
   
   while(rr_list->next != NULL) {
      dup_counter = 0;
      
      while(rr2_list->next != NULL) {
         if (strncmp(rr_list->name_conv, rr2_list->name_conv, strlen(rr_list->name_conv) + 1) == 0 && \
            strncmp(rr_list->name_short, rr2_list->name_short, strlen(rr_list->name_short)) != 0 && \
            rr_list->level == rr2_list->level) {
            
            printf("Info: Duplicate: %s and %s -> %s\n", rr_list->name_path, rr2_list->name_path, \
            add_duplicate_couter(rr2_list->name_conv, dup_counter, &rr2_list->name_conv_len));

            dup_counter++;
         }
         
         rr2_list = rr2_list->next;
      }
      rr2_list = list;
      
      rr_list = rr_list->next;
   }
}

static char *add_duplicate_couter(char *input, int counter, uint8_t *input_len) {
   char *name = NULL;
   char *ext = NULL;
   char *offset = NULL;
   char prepad_nulls[3];
   char str_counter[4];
   char save_ext[4];
   size_t name_len = 0;
   int counter_digits = 0;
   
   memset(prepad_nulls, 0, sizeof(prepad_nulls));
   memset(str_counter, 0, sizeof(str_counter));
   memset(save_ext, 0, sizeof(save_ext));
   
   /* Can't be done with memset. Valgrind doesn't like it */
   strncpy(prepad_nulls, "000", 4);
   
   /* Get pointers to name and extension */
   filename_explode(input, &name, &ext, CONV_ISO9660);
   name_len = strlen(name);
   
   /* 
    * Save extension to stack.
    * Otherwise it would be overwritten if filename is shorter than
    * 4 bytes (this number is just a guess ;-) ), by:
    * snprintf(offset, 5, "_%s", str_counter);
    */
   if (ext != NULL)
      strncpy(save_ext, ext, sizeof(save_ext)-1);
   
   if (counter > 999)
      printf("Warning: Duplicity counter too large, file might be unreadable on target ISO\n");
   
   /* For padding purposes, how many '0' chars should be prepadded */
   counter_digits = log10(counter) + 1;
   
   /* Create counter string (e.g. 001, 066, 123 ...)*/
   snprintf(str_counter, sizeof(str_counter), "%.*s%d", (int) (sizeof(prepad_nulls) - counter_digits), prepad_nulls, counter);
   
   /* 
    * Everything after 4th (including) will be replaced by couter string.
    * This space is needed to accomodate counter string.
    */
   if (name_len >= 4)
      offset = name + 4;
   else
      offset = name + name_len;
   
   /* 
    * Put couter string directly into file_list structure 
    * snprintf() outputs '\0' as last character
    */
   snprintf(offset, 5, "_%s", str_counter);
   
   /* If there was a filename extension, add it back to filename */
   if (ext != NULL)
      snprintf(offset + sizeof(str_counter), 1 + strlen(save_ext) + 1, ".%s", save_ext);
   
   *input_len = strlen(name);
   
   return name;
}  

uint8_t convert_name(char *input, char *output, enum conv_type_l type) {
   char *name = NULL;
   char *ext = NULL;
   char rr_input[MAX_DIR_STR_LEN];
   int name_len = 0;
   int ext_len = 0;
   
   memset(rr_input, 0, sizeof(rr_input));
   strncpy(rr_input, input, sizeof(rr_input));
   
   filename_explode(rr_input, &name, &ext, type);
   
   name_len = conv_rules(name, 8);
   ext_len = conv_rules(ext, 3);
   
   /* 
    * WARNING:
    * *output must be large enough 
    * otherwise SEGFAULT is inevitable
    */
   strncat(output, name, name_len);

   if (ext != NULL)
      snprintf(output + name_len, ext_len + 2, ".%s", ext);
   
   return strlen(output);
}

static void filename_explode(char *input, char **oname, char **oext, enum conv_type_l type) {
   int input_len = strlen(input);
   int i = 0;
   
   /* When 8.3 filenames are in use, filename can't start with '.' */
   if (*input == '.' && type == CONV_ISO9660)
      *input = '_';
   
   /* Set pointer to last character in the string */
   input += input_len - 1;
   
   /* Check if we have dot (.) separataor present */
   for (i = 0; i < input_len; i++) {
      if (*input == '.') {
         *oext = input + 1;                        // Extension will be set one byte behind first separator from the end
         *oname = input - (input_len - i) + 1;     // File name is set here
         *input = '\0';                            // Replace separator with '\0'
         break;                                    // No need to look further
      }
      
      input--;                                     // Move to next character
   }
   
   /* No separator was found, file name will equal to input and extension will be NULL */
   if (*oname == NULL)
      *oname = input + 1;
}

static size_t conv_rules(char *input, int max_len) {
   size_t input_len = 0;
   size_t i = 0;
   
   /* Mostly because filename extension might be missing */
   if (input != NULL) {
      input_len = strlen(input);
      
      if (input_len > max_len)
         input_len = max_len;
      
      for (i = 0; i < input_len; i++) {
         if (*input >= 97 && *input <= 122)
            *input -= 32;                                                              // Convert lower case characters to upper case
         else if ((*input >= 65 && *input <= 90) || (*input >= 48 && *input <= 57)) {
            // Do not convert this character set
         }
         else
            *input = '_';                                                              // Everything else will become '_'
         
         input++;
      }
      
      *input = '\0';
   }
   
   return input_len;
}
