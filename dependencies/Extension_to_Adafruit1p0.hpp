class Custom_Button{  /// This class is an extension to functionality of Adafruit_GFX. It is analogical to Adafruit_GFX_Button class in usage
  private:            /// with that difference that it displays 2 bitmaps (drawButton(true/false)) as the button. Bitmaps have to be PROGMEM'ed
    int16_t Top_Left_X;     /// as arrays of 16 bit (RGB 5/6/5) with element number of width*height
    int16_t Top_Left_Y;     /// Alternatively it can also be a monochrome bitmap of uint8_t with true/false colors defined in initButton function
    const uint16_t* Bitmap_Progmem_Pos;     /// You can recall initButton function multiple times during execution of a program
    const uint8_t* Mask_Progmem_Pos;        /// This comes with example of usage and video tutorial.
    const uint16_t* Bitmap_Progmem_Neg;     /// by Gen0me. Free to use. github.com/63n0m3/Custom_Button/ btc: 1A8Gqs2JwdjuAfFZ4cnM4CWy9hGFzsCpzK
    const uint8_t* Mask_Progmem_Neg;
    const uint8_t* Mon_Bitmap_Progmem;
    uint16_t Color_True;
    uint16_t Color_False;
    Adafruit_GFX* Display;
    bool currstate, laststate;

  public:
    unsigned long Last_Time_Used_millis;    /// You can use this varieable to time multiple pressings on one hold
    int16_t Width;
    int16_t Height;

    void initButton(int16_t top_left_x, int16_t top_left_y, const uint16_t bitmap_progmem_pos[], const uint8_t mask_progmem_pos[],
    const uint16_t bitmap_progmem_neg[], const uint8_t mask_progmem_neg[], int16_t width, int16_t height, Adafruit_GFX* display){
      Top_Left_X = top_left_x - (width/2);                          /// mask controls transparency, if mask_progmem_pointer points to NULL, there is no
      Top_Left_Y = top_left_y - (height/2);                          /// transarency and different drawing function is used (possibly faster)
      Bitmap_Progmem_Pos = bitmap_progmem_pos;          /// 16-bit image (RGB 5/6/5) with a 1-bit mask (set bits = opaque, unset bits = clear)
      Mask_Progmem_Pos = mask_progmem_pos;
      Bitmap_Progmem_Neg = bitmap_progmem_neg;
      Mask_Progmem_Neg = mask_progmem_neg;
      Mon_Bitmap_Progmem = NULL;
      Width = width;
      Height = height;
      Display = display;
      currstate = false;
      laststate = false;
      Last_Time_Used_millis = millis();
    }
                                                                  ///For 1 bit per pixel(monochrome bitmaps)
    void initButton(int16_t top_left_x, int16_t top_left_y, const uint8_t mon_bitmap_progmem[], uint16_t color_true, uint16_t color_false, int16_t width, int16_t height, MCUFRIEND_kbv* display){
      Top_Left_X = top_left_x - (width/2);
      Top_Left_Y = top_left_y - (height/2);
      Bitmap_Progmem_Pos = NULL;
      Mask_Progmem_Pos = NULL;
      Bitmap_Progmem_Neg = NULL;
      Mask_Progmem_Neg = NULL;
      Mon_Bitmap_Progmem = mon_bitmap_progmem;
      Color_True = color_true;
      Color_False = color_false;
      Width = width;
      Height = height;
      Display = display;
      currstate = false;
      laststate = false;
      Last_Time_Used_millis = millis();
    }
    void drawButton(boolean used){
      Last_Time_Used_millis = millis();
      if (used == true){
        if (Mon_Bitmap_Progmem !=NULL){
          Display->drawBitmap(Top_Left_X, Top_Left_Y, Mon_Bitmap_Progmem, Width, Height, Color_True);
        }
        else{
          if (Mask_Progmem_Pos == NULL)
            Display->drawRGBBitmap(Top_Left_X, Top_Left_Y, Bitmap_Progmem_Pos, Width, Height);
          else Display->drawRGBBitmap(Top_Left_X, Top_Left_Y, Bitmap_Progmem_Pos, Mask_Progmem_Pos, Width, Height);
        }
      }
      else {
        if (Mon_Bitmap_Progmem !=NULL){
          Display->drawBitmap(Top_Left_X, Top_Left_Y, Mon_Bitmap_Progmem, Width, Height, Color_False);
        }
        else{
          if (Mask_Progmem_Neg == NULL)
            Display->drawRGBBitmap(Top_Left_X, Top_Left_Y, Bitmap_Progmem_Neg, Width, Height);
          else Display->drawRGBBitmap(Top_Left_X, Top_Left_Y, Bitmap_Progmem_Neg, Mask_Progmem_Neg, Width, Height);
        }
      }
    }
    bool contains(int16_t point_x, int16_t point_y){
      if ((point_x > Top_Left_X && point_x < Top_Left_X + Width) && point_y > Top_Left_Y && point_y < Top_Left_Y + Height) return true;
      else return false;
    }
    void press(bool p){
      laststate = currstate;
      currstate = p;
    }
    bool justPressed(){
      return (currstate && !laststate);
    }
    bool justReleased(){
      return (!currstate && laststate);
    }
    void Change_Position(int16_t mid_x, int16_t mid_y){
      Top_Left_X = mid_x - (Width/2);
      Top_Left_Y = mid_y - (Height/2);
    }
};
