/*
 * SL.c
 * 
 * Version:       0.2.0
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

#include "SL.h"

int SL_create(char *input, unsigned char **output, int *output_len) {
   struct buffer_data_t buff;
   unsigned char *split_record = NULL;                         // Used only during split record
   int split_record_len = 0;                                   // Used only during split record
   int rv = E_OK;
   
   memset(&buff, 0, sizeof(struct buffer_data_t));
   
#if (DEBUG == 1)
   printf("DEBUG: %s(): Allocating new space: %d bytes.\n", __func__, SL_PREALLOC);
#endif
   
   if ((buff.poutput_start = (unsigned char *) malloc(SL_PREALLOC)) == NULL) {
      return Gdisplay_message(E_MALLOC, __func__);
   }
   else
      memset(buff.poutput_start, 0, SL_PREALLOC);

   /* Insert known data to entry header */
   buff.poutput_start[0] = 'S';
   buff.poutput_start[1] = 'L';
   buff.poutput_start[3] = 0x01;

   buff.total_len = SL_HEADER_SIZE;
   buff.pbuff = buff.poutput_start + buff.total_len;           // Move pointer behind system use entry header
   buff.pSL_len = buff.poutput_start + 2;                      // Position of SL record length
   buff.pflags = buff.poutput_start + 4;                       // Position of SL record flag
   buff.pcomponent_len = buff.poutput_start + 6;               // Position of component record len
   buff.blocks_allocated = 1;
   
   /* Check for type of first component record */
   if (*input == '/') {
      *(buff).pbuff++ = ROOT;                   // Flag
      buff.pbuff++;                             // Length
   }
   else if (*input == '.' && *(input + 1) == '.' && *(input + 2) == '/') {
      *(buff).pbuff++ = PARENT;                 // Flag
      buff.pbuff++;                             // Length
      input += 2;                               // move pointer to first '/' in the stream
   }
   else if (*input == '.' && *(input + 1) == '/') {
      *(buff).pbuff++ = CURRENT;                // Flag
      buff.pbuff++;                             // Length
      input += 1;                               // move pointer to first '/' in the stream
   }
   else {
      buff.pbuff++;                             // Flag
      buff.pbuff++;                             // Length
   }
   
   buff.total_len += 2;                         // Allign total length
   
   /* Process input string until 0 is found */
   while(*input) {
      if (*input == '/') {
         
         /* Dont add any more bytes when end is near */
         if (*(input + 1) == 0)
            break;
         
         input++;
         
         /* There are multiple '/' in a row, create entry for them */
         if (*input == '/') {
            
            if ((rv = check_realloc_main(&buff, 2)) != E_OK)
               goto cleanup;
            
            *(buff).pbuff++ = ROOT;           // Flag
            *(buff).pbuff++ = 0x00;           // Lenth, have hardcoded zero
            
            continue;
         }
         
         /* Link to parent directory */
         if (*input == '.' && *(input + 1) == '.' && (*(input + 2) == '/' || *(input + 2) == 0)) {
            input += 2;
            
            if ((rv = check_realloc_main(&buff, 2)) != E_OK)
               goto cleanup;
            
            *(buff).pbuff++ = PARENT;
            *(buff).pbuff++ = 0x00;
            
            continue;
         }
         
         /* Link to current directory */
         if (*input == '.' && (*(input + 1) == '/' || *(input + 1) == 0)) {
            input += 1;
            
            if ((rv = check_realloc_main(&buff, 2)) != E_OK)
               goto cleanup;
            
            *(buff).pbuff++ = CURRENT;
            *(buff).pbuff++ = 0x00;
            
            continue;
         }
         
         if ((rv = check_realloc_main(&buff, 2)) != E_OK)
            goto cleanup;
         
         *(buff).pbuff++ = 0x00;                            // Flag, can be updated with split_SL_record() if needed
         
         *(buff).pcomponent_len = buff.component_len;       // Write length of previous component
         
         buff.pcomponent_len = buff.pbuff;                  // Save position of component length
         
         *(buff).pbuff++ = 0x00;                            // Length placeholder
         
         buff.component_len = 0;
      }
      
      if ((rv = check_realloc_main(&buff, 1)) != E_OK)
         goto cleanup;
     
      buff.component_len++;
      
      /* Single contious compont can't be longer than 256 bytes */
      if (buff.component_len > 255) {
         printf("Error: %s(): Invalid symlink\n", __func__);
         rv = E_LNKFAIL;
         goto cleanup;
      }
      
      *(buff).pbuff++ = *input++;                           // Assign char to output buffer
   }
   
   /* Write length of last valid link component */
   *(buff).pcomponent_len = buff.component_len;
   *(buff).pSL_len = buff.total_len;
   
   /* Split SL record to smaller chunks if necessary */
   if (buff.total_len > SL_MAX_LEN) {

      if ((rv = split_SL_record(buff.poutput_start, buff.total_len, &split_record, &split_record_len)) != E_OK) {
         goto cleanup;
      }
      else {
         *output = split_record;
         *output_len = split_record_len;
         free(buff.poutput_start);
      }
   }
   else {
      /* Set output pointer to correct value if needed */
      *output = buff.poutput_start;
      *output_len = buff.total_len;
   }

#if (DEBUG == 1)
   printf("DEBUG: %s(): Original len: %d bytes\n", __func__, buff.total_len);
   printf("DEBUG: %s(): Split len: %d bytes\n", __func__, split_record_len);
#endif
   
cleanup:
   if (rv != E_OK) {
      if (buff.poutput_start != NULL)
         free(buff.poutput_start);
   }
   
   return rv;
}

static int split_SL_record(unsigned char *input, int input_len, unsigned char **output, int *output_len) {
   struct save_data_t save_data;                                              // Structure to save pointers
   unsigned char *rr_output = NULL;
   int rv = E_OK;
   int SL_len = 0;
   int part_component_len = 0;                                                // Follows component length in case of split
   int len2split_remain = SL_MAX_LEN;                                         // Controls if and when should be component split
   int total_len = 0;                                                         // Total lenght of splited SL record
   int rr = 0;
   uint8_t component_len = 0;                                                 // Original component length
   uint8_t component_flag = 0;
   uint8_t add = 0;
   
   memset(&save_data, 0, sizeof(save_data));
   save_data.inital_alloc = (input_len * 2);                                  // Just a rough guess
   save_data.mem_free = save_data.inital_alloc - SL_HEADER_SIZE;              // At least SL_HEADER_SIZE will be used
   save_data.blocks_allocated = 1;
   
#if (DEBUG == 1)
   printf("DEBUG: %s(): Allocating new space: %zu bytes.\n", __func__, save_data.inital_alloc);
#endif
   
   if ((save_data.start = (unsigned char *) malloc(save_data.inital_alloc)) == NULL) {
      return Gdisplay_message(E_MALLOC, __func__);
   }
   else {
      rr_output = save_data.start;
      memset(rr_output, 0, save_data.inital_alloc);
      memcpy((void *) rr_output, (void *) input, SL_HEADER_SIZE);             // Copy SL header
      
      save_data.SLlen_save = rr_output + 2;                                   // Save position of SL record lenght
      save_data.SLflag_save = rr_output + 4;                                  // Save position of SL record flags
      
      /* Update pointer and sizes due previous memcpy() */
      input += SL_HEADER_SIZE;
      rr_output += SL_HEADER_SIZE;
      input_len -= SL_HEADER_SIZE;
      len2split_remain -= SL_HEADER_SIZE;
      SL_len += SL_HEADER_SIZE;
      total_len += SL_HEADER_SIZE;
      
      while(input_len > 0) {
         component_flag = *(input++);                                         // Store flag value
         component_len = *(input++);                                          // Store length value
         input_len -= 2;
         
         /* Component name can't be separated from its flags and length */
         add = (component_len > 0) ? 1 : 0;
         
         /* Split record */
         if (len2split_remain - (2 + add) < 0) {
            *save_data.SLflag_save = 1;
            *save_data.SLlen_save = SL_len;
            *save_data.component_len_save = part_component_len;
            
            if ((rv = check_realloc_part(&save_data, 5, &rr_output)) != E_OK)
               goto cleanup;
         
            write_SL_header(&rr_output, &save_data);
            
            /* 
             * Following section insures that '/' (0x08 0x00) is inserted in front 
             * of start of every component record name, 
             * but only if new SL record is created.
             * it is not very clear what RRIP standard says about this, but some clients had 
             * problem to correctly interpret symlink without it.
             */
            if (component_len > 0) {
               if ((rv = check_realloc_part(&save_data, 2, &rr_output)) != E_OK)
                  goto cleanup;
               
               write_data(ROOT, &rr_output, &len2split_remain);   // Write root record
               rr_output++;                                       // Insert length
               
               rr = 2;
            }
            
            len2split_remain = SL_MAX_LEN - SL_HEADER_SIZE - rr;
            SL_len = SL_HEADER_SIZE + rr;
            total_len += (SL_HEADER_SIZE + rr);
            
            rr = 0;
         }
         
         if ((rv = check_realloc_part(&save_data, 2, &rr_output)) != E_OK)
            goto cleanup;
         
         /* Record component flags and lenght */
         save_data.component_flag_save = write_data(component_flag, &rr_output, &len2split_remain);
         save_data.component_len_save = write_data(component_len, &rr_output, &len2split_remain);
         
         SL_len += 2;
         total_len += 2;
         
         part_component_len = 0;
         
         /* Record component data (name) */
         while(component_len--) {
            
            if (len2split_remain == 0) {
               *save_data.SLflag_save = 1;
               *save_data.SLlen_save = SL_len;
               *save_data.component_len_save = part_component_len;
               *save_data.component_flag_save = 1;
               
               if ((rv = check_realloc_part(&save_data, 7, &rr_output)) != E_OK)
                  goto cleanup;
               
               write_SL_header(&rr_output, &save_data);
               
               save_data.component_flag_save = rr_output++;
               save_data.component_len_save = rr_output++;
               
               /* +/- 2 to allign with component_flag_save & component_len_save */
               SL_len = (SL_HEADER_SIZE + 2);
               len2split_remain = (SL_MAX_LEN - SL_HEADER_SIZE - 2);
               total_len += (SL_HEADER_SIZE + 2);
               
               part_component_len = 0;
            }
            
            if ((rv = check_realloc_part(&save_data, 1, &rr_output)) != E_OK)
               goto cleanup;
            
            write_data(*(input++), &rr_output, &len2split_remain);
            
            input_len--;
            SL_len++;
            part_component_len++;
            total_len++;
         }
         
         *save_data.component_len_save = part_component_len;
         *save_data.SLlen_save = SL_len;
      }
      
      *output = save_data.start;
      *output_len = total_len;
   }

cleanup:
   if (rv != E_OK) {
      if (save_data.start != NULL)
         free(save_data.start);
   }
   
   return rv;
}

static unsigned char *write_data(char source, unsigned char **dest, int *remain) {
   **dest = source;
   
   (*remain)--;
   (*dest)++;
   return (*dest) - 1;
}

static void write_SL_header(unsigned char **head, struct save_data_t *mem_save_data) {
   unsigned char *rr_head = *head;
   
   *rr_head++ = 'S';
   *rr_head++ = 'L';
   mem_save_data->SLlen_save = rr_head;
   *rr_head++ = 0;                           // Length
   *rr_head++ = VERSION;
    mem_save_data->SLflag_save = rr_head;
   *rr_head++ = 0;                           // Flag
   
   *head = rr_head;
}

static int check_realloc_main(struct buffer_data_t *buff, int increment_by) {
   int rr_total_len = buff->total_len;                           // Make things a bit obscure ...
   int rv = E_OK;
   unsigned long int pcomponent_len_off = 0;
   unsigned long int pSL_len_off = 0;
   
   rr_total_len += increment_by;
    
   /* Check if we need to allocate additional memory chunk */
   if (rr_total_len > buff->blocks_allocated * SL_PREALLOC) {
      unsigned char *rr = NULL;
      
      pcomponent_len_off = buff->pcomponent_len - buff->poutput_start;
      pSL_len_off = buff->pSL_len - buff->poutput_start;

#if (DEBUG == 1)
      printf("DEBUG: %s(): Allocating additional space: %d bytes\n", __func__, (buff->blocks_allocated + 1) * SL_PREALLOC);
#endif      

      if ((rr = realloc(buff->poutput_start, ++buff->blocks_allocated * SL_PREALLOC)) == NULL) {
         return Gdisplay_message(E_MALLOC, "check_realloc");
      }
      else {
         memset(rr + buff->total_len, 0, (buff->blocks_allocated * SL_PREALLOC) - buff->total_len);
         
         /* Update pointers with new memory address */
         buff->poutput_start = rr;
         buff->pSL_len = rr + pSL_len_off;
         buff->pbuff = rr + buff->total_len;
         buff->pcomponent_len = rr + pcomponent_len_off;
         buff->pflags = buff->pSL_len + 2;
      }
   }
   
   buff->total_len = rr_total_len;
   
   return rv;
}

static int check_realloc_part(struct save_data_t *save_data, int ammount, unsigned char **head) {
   unsigned char *rr = NULL;
   int rv = E_OK;
   unsigned long int SLflag_off = 0;
   unsigned long int SLlen_off = 0;
   unsigned long int component_flag_off = 0;
   unsigned long int component_len_off = 0;
   unsigned long int head_off = 0;
   
   save_data->mem_free -= ammount;
   
   if (save_data->mem_free < 0) {
      
      /* 
       * Save absolute memory position of save_data members 
       * for later pointer restore after realloc() 
       */
      head_off = *head - save_data->start;
      SLflag_off = save_data->SLflag_save - save_data->start;
      SLlen_off = save_data->SLlen_save - save_data->start;
      
      if (save_data->component_flag_save != NULL)
         component_flag_off = save_data->component_flag_save - save_data->start;
      
      if (save_data->component_len_save != NULL)
         component_len_off = save_data->component_len_save - save_data->start;

#if (DEBUG == 1)
         printf("DEBUG: %s(): Allocating additional space %zu bytes\n", __func__, (save_data->blocks_allocated + 1) * save_data->inital_alloc);
#endif
      
      /* Try to reallocate memory */
      if ((rr = realloc(save_data->start, ++save_data->blocks_allocated * save_data->inital_alloc)) == NULL) {
         return Gdisplay_message(E_MALLOC, __func__);
      }
      else {
         memset(rr + ((save_data->blocks_allocated - 1) * save_data->inital_alloc), 0, save_data->inital_alloc);
         save_data->start = rr;
         
         /* Restore previous location of pointers */
         *head = save_data->start + head_off;
         save_data->SLflag_save = save_data->start + SLflag_off;
         save_data->SLlen_save = save_data->start + SLlen_off;
         
         /* Component flag and length were not set */
         if (component_flag_off == 0)
            save_data->component_flag_save = NULL;
         else
            save_data->component_flag_save = save_data->start + component_flag_off;
         
         if (component_len_off == 0)
            save_data->component_len_save = NULL;
         else
            save_data->component_len_save = save_data->start + component_len_off;
         
         save_data->mem_free += save_data->inital_alloc;
      }
   }
   
   return rv;
}
