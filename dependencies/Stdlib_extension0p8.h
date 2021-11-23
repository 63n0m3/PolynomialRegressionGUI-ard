                                                                    /// For example you can use this function to get instead of -1.7664e-01 -> -0.17664 or 6.8787e+01 -> 68.787 or just keep the e notation depending on the set of parameters
                                                                    /// This is the extended dtostre function. It displays floating numbers in a more human frendly way
                                                                    /// 4 first arguments are the same as in original dtostre function, however the third argument gets different meaning
                                                                    /// as it also describes number of displayed places if function uses more human friendly notation format
                                                                    /// Two last arguments describe how many places can dot be moved right(-1) and left(-1) to write the string array in human friendly notation
                                                                    /// Programmer should provide char array with enough length, and function will return the actuall length of written characters
                                                                    /// provide max_int_places that 1 <= max_int_places <= decimals_after_dot+1
                                                                    /// by Gen0me, https://github.com/63n0m3/Stdlib_extension/   btc: bc1q5u49442fc08kugxp70xcer322njh3l5hplzw2k
uint16_t dtostren0(double number, char* out_array, unsigned char decimals_after_dot, unsigned char flags, int16_t max_int_places, int16_t max_aftdec0){
  dtostre(number, out_array, decimals_after_dot, flags);
  int16_t is_minus = 0;
  if (flags & DTOSTR_ALWAYS_SIGN == DTOSTR_ALWAYS_SIGN) is_minus = 1;
  else if (out_array[0] == '-') is_minus = 1;
  int16_t e10 = (int16_t)out_array[4+decimals_after_dot+is_minus]-48;
  int16_t e1 = (int16_t)out_array[5+decimals_after_dot+is_minus]-48;
  if (e10>=10 || e1 >= 10 || e10<0 || e1<0) return 0;
  int16_t expo = e10*10 + e1;
  uint16_t str_char_length = is_minus+2+decimals_after_dot+4;
  if (out_array[3+decimals_after_dot+is_minus] == '-') expo = -expo;
  if (expo>0 && expo<max_int_places){
//    out_array[2+decimals_after_dot+is_minus] = '\0';
    int16_t j=1+is_minus;
    for (; j<=expo+is_minus; j++)
      out_array[j] = out_array[j+1];
    out_array[j] = '.';
    str_char_length -= 4;
    for(int16_t i=str_char_length-1; ; i--){  /// cleaning after . zeros
      if (out_array[i] == '0') ;
      else if (out_array[i] == '.'){
        out_array[i] = '\0';
        str_char_length = i;
        break;
      }
      else {
        out_array[i+1] = '\0';
        str_char_length = i+1;
        break;
      }
    }
  }
  else if (expo<0 && -expo<max_aftdec0){
    int16_t j=1+is_minus+decimals_after_dot;
    out_array[j-expo+1] = '\0';
    str_char_length = j-expo+1;
    for (; j>=1; j--)
      out_array[j-expo] = out_array[j];
    out_array[is_minus+1-expo] = out_array[is_minus];
    for(int16_t i=is_minus-expo; i>=is_minus; i--)
      out_array[i] = '0';
    out_array[is_minus+1] = '.';
    for(int16_t i=str_char_length-1; ; i--){  /// cleaning after . zeros
      if (out_array[i] == '0') ;
      else if (out_array[i] == '.'){
        out_array[i] = '\0';
        str_char_length = i;
        break;
      }
      else {
        out_array[i+1] = '\0';
        str_char_length = i+1;
        break;
      }
    }
  }
  else {
    if (e10 == 0){
      if (e1 == 0){
  //      out_array[2+decimals_after_dot+is_minus] = '\0';
        str_char_length -=4;
      }
      else{
        out_array[4+decimals_after_dot+is_minus] = out_array[5+decimals_after_dot+is_minus];
  //      out_array[5+decimals_after_dot+is_minus] = '\0';
        str_char_length--;
      }
    }
      int16_t c1 = 0;                        /// cleaning after . zeros
      for (int16_t i=is_minus+1+decimals_after_dot; ; i--){
        if(out_array[i] == '0')
          c1++;
        else if(out_array[i] == '.'){
          c1++;
          break;
        }
        else break;
      }
      if (c1>0){
        for (int16_t i=is_minus+2+decimals_after_dot-c1; i<str_char_length-c1; i++){
          out_array[i] = out_array[i+c1];
        }
        str_char_length -= c1;
      }
      out_array[str_char_length] = '\0';
    }
  return str_char_length;
}
