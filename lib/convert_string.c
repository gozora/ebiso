void conv2level1(char *input, char *output) {
   size_t input_len = strlen(input);
   uint16_t max_len = 12;
   uint8_t dot = 0;
   
   if(input_len > max_len)
      input_len = max_len;

   strncpy(output, input, input_len);
   
   while(input_len--) {
      if (*output >= 97 && *output <= 122)
         *output -= 32;
      else if ((*output >= 65 && *output <= 90) || (*output >= 48 && *output <= 57)) {
         // Nothing to be done here
      }
      else if (*output == '.' && dot == 0) {
         dot = 1;
         input_len = 3;
         *(output+4) = '\0';
      }
      else
         *output = '_';
      
      output++;
   }
}
