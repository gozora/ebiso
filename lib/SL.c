/*
 * SL.c
 * 
 * Version:       0.1.1
 * 
 * Release date:  15.11.2015
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

#include "SL.h"

int SL_create(char *input, unsigned char **output) {
   int rv = 0;
   unsigned char *pbuff = *output;
   unsigned char *plen = NULL;
   uint8_t part_len = 0;
   int total_len = 5;              // Skip system use entry header
   int rr_len_save = 0;
   
   pbuff += total_len;                 // Move pointer behind system use entry header
   
   /* Check first character in string */
   if (*input == '/') {
      *pbuff++ = ROOT;                 // flag
      pbuff++;                         // len 0
   }
   else if (*input == '.' && *(input + 1) == '.' && *(input + 2) == '/') {
      *pbuff++ = PARENT;               // flag
      pbuff++;                         // len 0
      input += 2;                      // move pointer to first '/' in the stream
   }
   else if (*input == '.' && *(input + 1) == '/') {
      *pbuff++ = CURRENT;              // flag
      pbuff++;                         // len 0
      input += 1;                      // move pointer to first '/' in the stream
   }
   else {
      pbuff++;                         // flag
      plen = pbuff;                    // save length location
      pbuff++;                         // len 0
   }
   
   rr_len_save = total_len + 1;
   total_len += 2;                     // allign total counter
   
   /* Process remaining character in string until 0 is found */
   while(*input) {
      
      if (*input == '/') {
         
         /* Dont add bytes when end is near */
         if (*(input + 1) == 0)
            break;
         
         input++;
         
         /* There are multiple '/' in a row, create entry for them */
         if (*input == '/') {
            
            if ((rv = check_realloc(&total_len, 2, output)) < 0)
               return rv;
            else if (rv == 1) {
               pbuff = *output + (total_len - 2);
               
               if (plen != NULL)
                  plen = *output + rr_len_save;
            }
            
            *pbuff++ = ROOT;           // flag
            *pbuff++ = 0x00;           // lenth, have hardcoded zero
            
            continue;
         }
         
         /* Link to parent directory */
         if (*input == '.' && *(input + 1) == '.' && (*(input + 2) == '/' || *(input + 2) == 0)) {
            input += 2;
            
            if ((rv = check_realloc(&total_len, 2, output)) < 0)
               return rv;
            else if (rv == 1) {
               pbuff = *output + (total_len - 2);
               
               if (plen != NULL)
                  plen = *output + rr_len_save;
            }
            
            *pbuff++ = PARENT;
            *pbuff++ = 0x00;
            
            continue;
         }
         
         /* Link to current directory */
         if (*input == '.' && (*(input + 1) == '/' || *(input + 1) == 0)) {
            input += 1;
            
            if ((rv = check_realloc(&total_len, 2, output)) < 0)
               return rv;
            else if (rv == 1) {
               pbuff = *output + (total_len - 2);
               
               if (plen != NULL)
                  plen = *output + rr_len_save;
            }
            
            *pbuff++ = CURRENT;
            *pbuff++ = 0x00;
            
            continue;
         }
         
         if ((rv = check_realloc(&total_len, 2, output)) < 0)
            return rv;
         else if (rv == 1) {
            pbuff = *output + (total_len - 2);
            
            if (plen != NULL)
               plen = *output + rr_len_save;
         }
         
         *pbuff++ = 0x00;              // #adjust: flag, have hardcoded zero (for now)
         
         if (plen != NULL)
            *plen = part_len;          // assign lenght of previous record
         
         plen = pbuff;                 // save length position
         
         /* 
          * Save absolute memory position in case of realloc()
          * -1 is used because length was already increased by 2
          * in previous check_realloc() so correct position is
          * -1 byte.
          */
         rr_len_save = total_len - 1;
         
         *pbuff++ = 0x00;              // length placeholder
         
         part_len = 0;
      }
      
      if ((rv = check_realloc(&total_len, 1, output)) < 0)
         return rv;
      else if (rv == 1) {
         pbuff = *output + (total_len - 1);
         plen = *output + rr_len_save;
      }
      
      part_len++;
      
      *pbuff++ = *input++;             // assign char to output buffer
   }
   
   /* Write length of last valid link element */
   if (plen != NULL)
      *plen = part_len;
   
   (*output)[0] = 'S';
   (*output)[1] = 'L';
   (*output)[2] = total_len;
   (*output)[3] = 0x01;
   
   return total_len;
}

int check_realloc(int *total_len, int increment_by, unsigned char **output) {
   static int counter = 1;
   int rr_total_len = *total_len;                           // Sorry, I'm bad with pointers
   int rv = 0;
   
   /* Preparation for handling of longer links than 0xff bytes */
   rr_total_len += increment_by;
   
   /* This will be improoved in the future */
   if (rr_total_len > 0xff) {
      show_msg(MSG_LONGLINK, "check_realloc");
      rv = -2;
      return rv;
   }
   
   if (rr_total_len > counter * INIT_MALLOC) {
      unsigned char *rr = NULL;
      
      if ((rr = realloc(*output, ++counter * INIT_MALLOC)) == NULL) {
         show_msg(MSG_REALLOC, "check_realloc");
         rv = -1;
      }
      else {
#ifdef DEBUG
         printf("==== DEBUG: New space allocation success [%d]\n", counter * INIT_MALLOC);
#endif
         memset(rr + (rr_total_len - 1), 0, (counter * INIT_MALLOC) - (rr_total_len - 1));
         
         *output = rr;
         
         /* Signal calling function that update of pointers is necessary */
         rv = 1;
      }
   }
   
   *total_len = rr_total_len;
   
   return rv;
}

void show_msg(enum msg_l msg_id, char *calling_function) {
   switch(msg_id) {
      case MSG_MALLOC:
         printf("Error: %s(): Memory allocation failed.\n", calling_function);
      break;
      case MSG_REALLOC:
         printf("Error: %s(): Memory reallocation failed.\n", calling_function);
      break;
      case MSG_LONGLINK:
         printf("Error: %s(): Sorry, links longer than 255 bytes are not yet supported.\n", calling_function);
      break;
   }
}
