#define PRGUI_BLACK   0x0000
#define PRGUI_GREEN   0x07E0
#define PRGUI_CYAN    0x07FF
#define PRGUI_PINKRGB565 0b1111101000011011
#define PRGUI_YELLOW  0xFFE0
#define PRGUI_WHITE   0xFFFF

#include <dependencies/Stdlib_extension0p8.h>
#include <dependencies/Extension_to_Adafruit1p0.hpp>
#include <dependencies/PolynomialRegressionFloat.h>

void Write_To_Eeprom(uint16_t Addr, uint16_t Size_Bytes, byte* Var_Ptr){
  for(int16_t i=0; i<Size_Bytes; i++){
    byte byte_to_write = *(Var_Ptr+i);
    EEPROM.write(Addr+i, byte_to_write);
  }
}
void Read_Eeprom(uint16_t Addr, uint16_t Size_Bytes, byte* Write_To_Ptr){
  for(int16_t i=0; i<Size_Bytes; i++){
    byte read_byte = EEPROM.read(Addr+i);
    *(Write_To_Ptr+i) = read_byte;
  }
}

inline unsigned long Get_Current_Time_millis(){
  return millis();
}

inline bool Is_Point_Inside_Square(int16_t Sx_min, int16_t Sx_max, int16_t Sy_min, int16_t Sy_max, int16_t P_x, int16_t P_y){
  if (Sx_min < P_x && Sx_max > P_x)
    if (Sy_min < P_y && Sy_max > P_y)
      return true;
  return false;
}

#ifndef CH_TYPE
#error pre - #define CH_TYPE  as float or double
#endif
/*#ifndef CH_SIMPLE_BUTTONS
#warning pre - #define CH_SIMPLE_BUTTONS  if you want to save memory
#endif
*/
/** Overall structure looks like this
 *  There is a Polynomial_Regression function that does polynomial regression
 *  There is a Calibr_Device class that uses Polynomial_Regression function to fit data
 *  There is a Chart class that uses pointer to Calibr_Device and it is a graphical chart display class for calibraiting Calibr_Device
 *  There is a Chart_Gui class that inherits from Chart that is a proper user GUI
 *  It is written this way that the user can run Chart_Gui class on display and using physically touch screen add data points, setup coefficient function and save it to Eeprom
 *  After user can just run Calibr_Device class without screen, reading from EEPROM setup, using Load_From_Eeprom(..), and Calculate_Polynomial_At(..)
 *  discussion: https://www.avrfreaks.net/forum/tutdis-polynomial-regression-gui
 *  video tutorial: https://youtu.be/icBz3evB9uE
 *  by Gen0me, https://github.com/63n0m3/PolynomialRegressionGUI-ard    btc: bc1qn8xgw5rlfm7xnxnrxcqk0ulhrmrqjy07s3zmfp
 */

CH_TYPE Map (CH_TYPE x, CH_TYPE in_min, CH_TYPE in_max, CH_TYPE out_min, CH_TYPE out_max){
  return (x-in_min)*(out_max-out_min)/(in_max-in_min) + out_min;
}

class Calibr_Device {
protected:
  uint16_t EEprom_Start_Addr;
  uint16_t EEprom_Last_Addr;
public:             ///Those are public only for direct read acess to arrays, dont change those varieables and arrays. Class controls arrays memory allocation.
  int16_t Points_num;
  uint16_t Coefficients_Auto_Calculate_Num_Settings;    /// should be protected
  uint16_t Coefficients_Calculate_Num;
  CH_TYPE * X_points;
  CH_TYPE * Y_points;
  CH_TYPE* Polynomial_coefficients;

  Calibr_Device (){
    Points_num = 0;
    X_points = NULL;
    Y_points = NULL;
    Polynomial_coefficients = NULL;
    Validity_Check();
    Coefficients_Auto_Calculate_Num_Settings = 0xde39;
    EEprom_Start_Addr = 0;
    EEprom_Last_Addr = 0;
  }
  Calibr_Device (Calibr_Device &calibr_device){
    Coefficients_Auto_Calculate_Num_Settings = calibr_device.Coefficients_Auto_Calculate_Num_Settings;
    EEprom_Start_Addr = calibr_device.EEprom_Start_Addr;
    EEprom_Last_Addr = calibr_device.EEprom_Last_Addr;
    Points_num = calibr_device.Points_num;
    Coefficients_Calculate_Num = calibr_device.Coefficients_Calculate_Num;
    X_points = (CH_TYPE*) malloc(Points_num*sizeof(CH_TYPE));
    Y_points = (CH_TYPE*) malloc(Points_num*sizeof(CH_TYPE));
    if (calibr_device.Polynomial_coefficients != NULL) Polynomial_coefficients = (CH_TYPE*) malloc ((Coefficients_Calculate_Num+1)*sizeof(CH_TYPE));
    else Polynomial_coefficients = NULL;
    for (int i=0; i<Points_num; i++){
      X_points[i] = calibr_device.X_points[i];
      Y_points[i] = calibr_device.Y_points[i];
    }
    for (int i=0; i<=Coefficients_Calculate_Num; i++)
      Polynomial_coefficients[i] = calibr_device.Polynomial_coefficients[i];
  }
  ~Calibr_Device(){
    free(X_points);
    free(Y_points);
    free(Polynomial_coefficients);
  }

protected:
  inline void Validity_Check(){
    if (X_points == NULL || Y_points == NULL)   /// <------------------------------   Than its wrong, memory wasnt allocated, how to deal with it?
    return;
  }
public:
  void Set_Autodetermine_Polynomial_Coefficients_LSRdDPmOm1(uint16_t max_coeff){  /// minumum(Sum of square residuals/(data points - order - 1))

  }
  void Set_Autodetermine_Max_Coefficient_Number(uint16_t max_coeff){    /// This setups auto maximum coefficient determination in multiple of 8 ( 8 -> 8, 9->8 .. 15->8, 16->16, 24->24 etc)
    max_coeff = max_coeff/8;
    Coefficients_Auto_Calculate_Num_Settings = 0xde38 + max_coeff;
  }
  void Set_Coefficient_Number(uint16_t coeff_num){
    Coefficients_Auto_Calculate_Num_Settings = coeff_num;
  }
  void Calculate_Polynomial_Coefficients(){                               /// This function calculates coefficients based on previous setup from "Set_Autodetermine_Max_Coefficient_Number(..)" or "Set_Coefficient_Number()" This function is better fitted to use with GUI.
    if (Coefficients_Auto_Calculate_Num_Settings > 0xde38){     /// Auto settings
      uint16_t tmp = Coefficients_Auto_Calculate_Num_Settings - 0xde38;
      tmp *= 8;
      Coefficients_Calculate_Num = Find_Best_Fit_Coeff_Num(tmp);
      Polynomial_coefficients = (CH_TYPE*) realloc (Polynomial_coefficients, (Coefficients_Calculate_Num+1)*sizeof(CH_TYPE));
      PolynomialRegression(X_points, Y_points, Points_num, Coefficients_Calculate_Num, Polynomial_coefficients);
    }
    else{
      Coefficients_Calculate_Num = Coefficients_Auto_Calculate_Num_Settings;
      Polynomial_coefficients = (CH_TYPE*) realloc (Polynomial_coefficients, (Coefficients_Calculate_Num+1)*sizeof(CH_TYPE));
      PolynomialRegression(X_points, Y_points, Points_num, Coefficients_Calculate_Num, Polynomial_coefficients);
    }
  }
  void Calculate_Polynomial_Coefficients(uint16_t coefficients_calculate_Num){ /// This function calculates coefficients for manually inputed number. This function may be more practical to use on a well known model.
    Coefficients_Calculate_Num = coefficients_calculate_Num;
    Polynomial_coefficients = (CH_TYPE*) realloc (Polynomial_coefficients, (Coefficients_Calculate_Num+1)*sizeof(CH_TYPE));
    PolynomialRegression(X_points, Y_points, Points_num, Coefficients_Calculate_Num, Polynomial_coefficients);
  }
  int16_t Find_Best_Fit_Coeff_Num(uint16_t max_coefficients){         /// <-----------------------------------  does it work correctly?
    CH_TYPE tmp_coeff[max_coefficients];
    CH_TYPE sq_coeff_rests = PolynomialRegression(X_points, Y_points, Points_num, 1, Polynomial_coefficients);
    int16_t best_fit_num;
    for (int16_t i=2; i<max_coefficients; i++){
      CH_TYPE tmp = PolynomialRegression(X_points, Y_points, Points_num, i, tmp_coeff);
      if (tmp<sq_coeff_rests) {
        sq_coeff_rests = tmp;
        best_fit_num = i;
      }
    }
    return best_fit_num;
  }
  CH_TYPE Calculate_Polynomial_At(CH_TYPE at){ ///Must previously run Calculate_Polynomial_Coefficients()
    CH_TYPE tmp_sum = 0;
    for (int16_t i=0; i<=Coefficients_Calculate_Num; i++){
        tmp_sum += Polynomial_coefficients[i] * pow (at, i);
    }
    return tmp_sum;
  }
                                                              /// On the first Eeprom save use this function to set Save addresses, than with every eeprom load function gui-menu will remember addresses for save with bool Save_To_Eeprom(no args)
                                                              /// Before First save using Gui you need to run this function or Load_From_Eeprom(..) to set addresses
                                                              /// Returns true if not enough space, false on success
                                                              /// Keep in mind it is Last Address used, not Last Address + 1
  bool Save_To_Eeprom(uint16_t Start_Addr, uint16_t Last_Addr){    ///This was coded to minimize eeprom writes, however if eeprom runs out of write cycles it is recommended to increase starting address by 8
    if (Last_Addr-Start_Addr+1 < Get_Size_Required_For_Save()){
      return 1;
    }
    EEprom_Start_Addr = Start_Addr;
    EEprom_Last_Addr = Last_Addr;
    int16_t tmp1;
    Read_Eeprom(Start_Addr, 2, (byte*)(&tmp1));
    if (tmp1 != Last_Addr) Write_To_Eeprom(Start_Addr, 2, (byte*)(&Last_Addr));
    Read_Eeprom(Start_Addr+2, 2, (byte*)(&tmp1));
    if (tmp1 != Coefficients_Auto_Calculate_Num_Settings) Write_To_Eeprom(Start_Addr+2, 2, (byte*)(&Coefficients_Auto_Calculate_Num_Settings));
    Read_Eeprom(Start_Addr+4, 2, (byte*)(&tmp1));
    if (tmp1 != Points_num) Write_To_Eeprom(Start_Addr+4, 2, (byte*)(&Points_num));
    Read_Eeprom(Start_Addr+6, 2, (byte*)(&tmp1));
    if (tmp1 != Coefficients_Calculate_Num) Write_To_Eeprom(Start_Addr+6, 2, (byte*)(&Coefficients_Calculate_Num));
    for(int16_t i=0; i<Points_num; i++){
      CH_TYPE tmp;
      Read_Eeprom(Start_Addr+8 +2*i*sizeof(CH_TYPE), sizeof(CH_TYPE), (byte*)(&tmp));
      if (tmp != X_points[i]) Write_To_Eeprom(Start_Addr+8 +2*i*sizeof(CH_TYPE), sizeof(CH_TYPE), (byte*)(&X_points[i]));
      Read_Eeprom(Start_Addr+8 +(2*i+1)*sizeof(CH_TYPE), sizeof(CH_TYPE), (byte*)(&tmp));
      if (tmp != Y_points[i]) Write_To_Eeprom(Start_Addr+8 +(2*i+1)*sizeof(CH_TYPE), sizeof(CH_TYPE), (byte*)(&Y_points[i]));
    }
    for(int16_t i=0; i<=Coefficients_Calculate_Num; i++){
      CH_TYPE tmp;
      Read_Eeprom(Last_Addr-(sizeof(CH_TYPE))*(i+1)+1, sizeof(CH_TYPE), (byte*)(&tmp));
      if (tmp != Polynomial_coefficients[i]) Write_To_Eeprom(Last_Addr-(sizeof(CH_TYPE))*(i+1)+1, sizeof(CH_TYPE), (byte*)(&Polynomial_coefficients[i]));
    }
    return 0;
  }
  bool Save_To_Eeprom(){
    return Save_To_Eeprom(EEprom_Start_Addr, EEprom_Last_Addr);
  }
  bool Load_From_Eeprom(uint16_t Start_Addr){                             /// Function returns true if unsuccessfull, 0 successfull
    uint16_t Last_Addr;
    int16_t points_num = Points_num;
    uint16_t coefficients_Calculate_Num = Coefficients_Calculate_Num;
    uint16_t coefficients_Auto_Calculate_Num_Settings = Coefficients_Auto_Calculate_Num_Settings;

    Read_Eeprom(Start_Addr, 2, (byte*)(&Last_Addr));
    Read_Eeprom(Start_Addr+2, 2, (byte*)(&Coefficients_Auto_Calculate_Num_Settings));
    Read_Eeprom(Start_Addr+4, 2, (byte*)(&Points_num));
    Read_Eeprom(Start_Addr+6, 2, (byte*)(&Coefficients_Calculate_Num));
    if (Last_Addr+1-Start_Addr < Get_Size_Required_For_Save()){
      Coefficients_Auto_Calculate_Num_Settings = coefficients_Auto_Calculate_Num_Settings;
      Points_num = points_num;
      Coefficients_Calculate_Num = coefficients_Calculate_Num;
      return 1;
    }
    EEprom_Start_Addr = Start_Addr;
    EEprom_Last_Addr = Last_Addr;
    X_points = (CH_TYPE*) realloc(X_points, Points_num*sizeof(CH_TYPE));
    Y_points = (CH_TYPE*) realloc(Y_points, Points_num*sizeof(CH_TYPE));
    Polynomial_coefficients = (CH_TYPE*) realloc (Polynomial_coefficients, (Coefficients_Calculate_Num+1)*sizeof(CH_TYPE));
    for(int16_t i=0; i<Points_num; i++){
      Read_Eeprom(Start_Addr+8 +2*i*sizeof(CH_TYPE), sizeof(CH_TYPE), (byte*)(&X_points[i]));
      Read_Eeprom(Start_Addr+8 +(2*i+1)*sizeof(CH_TYPE), sizeof(CH_TYPE), (byte*)(&Y_points[i]));
    }
    for(int16_t i=0; i<=Coefficients_Calculate_Num; i++){
      Read_Eeprom(Last_Addr-(sizeof(CH_TYPE))*(i+1)+1, sizeof(CH_TYPE), (byte*)(&Polynomial_coefficients[i]));
    }
    return 0;
  }
  void Load_PointX_PointY_From_Arrays(uint16_t num_points, CH_TYPE* X_arr, CH_TYPE* Y_arr){
    Points_num = num_points;
    X_points = (CH_TYPE*) realloc(X_points, Points_num*sizeof(CH_TYPE));
    Y_points = (CH_TYPE*) realloc(Y_points, Points_num*sizeof(CH_TYPE));
    Validity_Check();
    for(int i=0; i<Points_num; i++){
      X_points[i] = X_arr[i];
      Y_points[i] = Y_arr[i];
    }
  }
  void Load_Coeff_From_Array(uint16_t num_coeff, CH_TYPE * coeff_arr){     /// Remember array should be of size: num_coeff+1 !!!
    Coefficients_Calculate_Num = num_coeff;
    Polynomial_coefficients = (CH_TYPE*) realloc(Polynomial_coefficients, (Coefficients_Calculate_Num+1)*sizeof(CH_TYPE));
    for(int i=0; i<=Coefficients_Calculate_Num; i++)
      Polynomial_coefficients[i] = coeff_arr[i];
  }
  int16_t Get_Size_Required_For_Save(){
    return 2*4 + sizeof(CH_TYPE)*2*Points_num +  sizeof(CH_TYPE)*(Coefficients_Calculate_Num+1);
  }

  inline int16_t Get_Points_num(){
    return Points_num;
  }
  void Add_Point(CH_TYPE x, CH_TYPE y){
    Points_num++;
    X_points = (CH_TYPE*) realloc(X_points, Points_num*sizeof(CH_TYPE));
    Y_points = (CH_TYPE*) realloc(Y_points, Points_num*sizeof(CH_TYPE));
    Validity_Check();
    X_points[Points_num-1] = x;
    Y_points[Points_num-1] = y;
    return;
  }
  void Change_Point_At(uint16_t at, CH_TYPE x, CH_TYPE y){
    if (at<Points_num){
      X_points[at] = x;
      Y_points[at] = y;
    }
  }
  void Delete_Point_At(uint16_t at){
    if (at<Points_num){
      for(int i=at; i<Points_num-1; i++){
        X_points[i] = X_points[i+1];
        Y_points[i] = Y_points[i+1];
      }
      Points_num--;
      if (Points_num == 0){
        free(X_points);
        free(Y_points);
        X_points = NULL;
        Y_points = NULL;
      }
      else {
        X_points = (CH_TYPE*) realloc(X_points, Points_num*sizeof(CH_TYPE));
        Y_points = (CH_TYPE*) realloc(Y_points, Points_num*sizeof(CH_TYPE));
      }
    }
  }
  void Delete_All_Points(){
    Points_num = 0;
    free(X_points);
    free(Y_points);
    X_points = NULL;
    Y_points = NULL;
  }
};

class Chart {
public:
  uint16_t X_axis_descriptions; /// Number of value descriptions for axies. feel free to change them and than redraw gui
  uint16_t Y_axis_descriptions;
  int16_t Point_Size;         /// How big the point is drawn. feel free to change.
  int16_t Width;              /// Those are theoretically public, but never tested. In theory each of those varieables shouldnt break if manually change. Just recalculate entire chart after the change.
  int16_t Height;
  int16_t Top_Left_X;
  int16_t Top_Left_Y;
  int16_t Point_Position_Square_TL_From_Left;
  int16_t Point_Position_Square_TL_From_Top;
  CH_TYPE X_highest_val;
  CH_TYPE X_lowest_val;
  CH_TYPE Y_highest_val;
  CH_TYPE Y_lowest_val;
  CH_TYPE Point_Selection_Max_Distance;
  int16_t Point_Position_Background_Width;   // This one shouldnt be changed while point is selected
  int16_t Point_Position_Background_Height;
  int16_t R_Menu_Top;
  int16_t R_Menu_Bot;
protected:
  int16_t* Polynomial_y_pixels;
  String Polynomial_Equation;
  uint16_t Currently_Selected_Point;
  Calibr_Device* Regression_Model;
  Adafruit_GFX* Tft;

public:
  Chart(uint16_t X, uint16_t Y, int16_t width, int16_t height, int16_t point_size, Adafruit_GFX* TFt){       // <------ HERE get rid of point_size
    Tft = TFt;
    if (Tft == NULL);
    else{
      Top_Left_X = X;
      Top_Left_Y = Y;
      Width = width;
      Height = height;
      Point_Position_Square_TL_From_Left = Width/2;
      Point_Position_Square_TL_From_Top = 22;
      Point_Position_Background_Width = 110;
      Point_Position_Background_Height = 40;
      R_Menu_Top = 34;
      R_Menu_Bot = height - 20;
      X_highest_val = 7;            ///Those are the highest and lowest values of chart display! (not points)
      X_lowest_val = 0;
      Y_highest_val = 10;
      Y_lowest_val = 0;
      Point_Size = point_size;
      Polynomial_y_pixels = NULL;
      Currently_Selected_Point = 0xffff;
      Point_Selection_Max_Distance = 45;
      X_axis_descriptions = 6;
      Y_axis_descriptions = 6;
    }
  }
  ~Chart(){
    free(Polynomial_y_pixels);
  }
  Add_Regression_Model(Calibr_Device* regression){
    Regression_Model = regression;
  }
protected:
  inline uint16_t Map_X_Point_to_Pixel(CH_TYPE x_point){
    return (uint16_t)Map(x_point, X_lowest_val, X_highest_val, Top_Left_X+12, Top_Left_X + Width-60);
  }
  inline uint16_t Map_Y_Point_to_Pixel(CH_TYPE y_point){
    return (uint16_t)Map(y_point, Y_highest_val, Y_lowest_val, Top_Left_Y+20, Top_Left_Y + Height-12);
  }
  inline CH_TYPE Map_X_Pixel_to_Point(uint16_t x_pixel){                                                  ///  <---- HERE Change float to CH_TYPE
    return Map(x_pixel, Top_Left_X+12, Top_Left_X + Width-60, X_lowest_val, X_highest_val);
  }
  inline CH_TYPE Map_Y_Pixel_to_Point(uint16_t y_pixel){
    return Map(y_pixel, Top_Left_Y+20, Top_Left_Y + Height-12, Y_highest_val, Y_lowest_val);
  }

  inline void Draw_Point(uint16_t point_x, uint16_t point_y){
    Tft->writePixel(point_x, point_y, PRGUI_WHITE);
    for(int16_t i=Point_Size; i>=1; i--){
      Tft->writePixel(point_x-i, point_y-i, PRGUI_WHITE);
      Tft->writePixel(point_x+i, point_y+i, PRGUI_WHITE);
      Tft->writePixel(point_x-i, point_y+i, PRGUI_WHITE);
      Tft->writePixel(point_x+i, point_y-i, PRGUI_WHITE);
    }
  }
  inline void Draw_Point(uint16_t point_num){
    Draw_Point(Map_X_Point_to_Pixel(Regression_Model->X_points[point_num]), Map_Y_Point_to_Pixel(Regression_Model->Y_points[point_num]));
  }
  void Calculate_Polynomial_Pixels(){   ///Must previously run Calculate_Polynomial_Coefficients()
    Polynomial_y_pixels = (int16_t*) realloc (Polynomial_y_pixels, (Width+1)*sizeof(uint16_t));
    for (int i=0; i<Width+1; i++)
      Polynomial_y_pixels[i] = Map_Y_Point_to_Pixel(Regression_Model->Calculate_Polynomial_At(Map_X_Pixel_to_Point(i+Top_Left_X)));
  }
  void Draw_Polynomial_At_X_Pixel(uint16_t x_pix){
    Tft->writePixel(x_pix, Polynomial_y_pixels[x_pix-Top_Left_X], PRGUI_GREEN);
  }
  void Draw_Square_Around_Point(uint16_t point_num){
    uint16_t point_x = Map_X_Point_to_Pixel(Regression_Model->X_points[point_num]);
    uint16_t point_y = Map_Y_Point_to_Pixel(Regression_Model->Y_points[point_num]);
    Tft->drawRect(point_x-3, point_y-3, 7, 7, PRGUI_YELLOW);
  }
  void Undraw_Point(uint16_t point_num){
    uint16_t point_x = Map_X_Point_to_Pixel(Regression_Model->X_points[point_num]);
    uint16_t point_y = Map_Y_Point_to_Pixel(Regression_Model->Y_points[point_num]);
    Tft->fillRect(point_x-4, point_y-4, 9, 9, PRGUI_BLACK);
    if (point_y>Top_Left_Y+Height-18)
      Draw_X_Axis();
    if (point_x<Top_Left_X+24)
      Draw_Y_Axis();
    for(uint16_t i=0; i<Regression_Model->Points_num; i++){
      if (i != point_num){
        if ((Map_X_Point_to_Pixel(Regression_Model->X_points[i]) > point_x-4 && Map_X_Point_to_Pixel(Regression_Model->X_points[i]) < point_x+4) && (Map_Y_Point_to_Pixel(Regression_Model->Y_points[i]) > point_y-4 && Map_Y_Point_to_Pixel(Regression_Model->Y_points[i]) < point_y+4))
          Draw_Point (Map_X_Point_to_Pixel(Regression_Model->X_points[i]), Map_Y_Point_to_Pixel(Regression_Model->Y_points[i]));
      }
    }
    for(int16_t i=point_x-4; i<=point_x+4; i++){
      if (i<Top_Left_X)
        i=Top_Left_X;
      if (i>Top_Left_X+Width)
        break;
      Draw_Polynomial_At_X_Pixel(i);
    }
  }
  void Draw_Point_Position(uint16_t point_num){
    char temstr[15];
    dtostren0(Regression_Model->X_points[point_num], temstr, 4, 0, 3, 3);
    char temstr2[15];
    dtostren0(Regression_Model->Y_points[point_num], temstr2, 4, 0, 3, 3);
    Tft->setTextColor(PRGUI_CYAN);
    Tft->setTextSize(2);
    {
      int16_t t1;
      int16_t t2;
      uint16_t t3;
      uint16_t tem_wid1;
      uint16_t tem_wid2;
      Tft->getTextBounds(temstr, 0, 0, &t1, &t2, &tem_wid1, &t3);
      Tft->getTextBounds(temstr2, 0, 0, &t1, &t2, &tem_wid2, &t3);
      Point_Position_Background_Width = (tem_wid1>tem_wid2 ? tem_wid1 : tem_wid2);
    }
    for(int16_t len = Point_Size+1; len<100;){
      len += random(3);
      randomSeed(millis());
      float ran_ang = (float)random(len*2)/(3.14*2);
      int16_t x_test = Map_X_Point_to_Pixel(Regression_Model->X_points[point_num]) + len*sin(ran_ang) - Top_Left_X;
 //     if (x_test+Point_Position_Background_Height+20 >= Width) continue;
      int16_t y_test = Map_Y_Point_to_Pixel(Regression_Model->Y_points[point_num]) + len*cos(ran_ang) - Top_Left_Y; //(Point_Position_Background_Height+Point_Size)*sin((float)x_test*3.14f/(float)len);
      if (ran_ang>0 && ran_ang<=3.14/2){      ///Right Bott
        if (x_test < 36) continue;      //Axis coliision
        if (y_test < R_Menu_Top) continue;      //Top collision
        if (y_test + Point_Position_Background_Height > Height - 20) continue;
        else if (y_test > R_Menu_Top && y_test < R_Menu_Bot-50-Point_Position_Background_Height){
          if (x_test + Point_Position_Background_Height > Width - 40) continue;
        }
        if (y_test > R_Menu_Bot-50-Point_Position_Background_Height){
          if (x_test + Point_Position_Background_Height > Width - 115) continue;
        }
  /*      tft.fillRect(x_test, y_test, 3, 3, BLUE);
        Tft->drawFastVLine(x_test, y_test, Point_Position_Background_Height, PRGUI_YELLOW);
        Tft->drawFastVLine(x_test+Point_Position_Background_Width, y_test, Point_Position_Background_Height, PRGUI_YELLOW);
        Tft->drawFastHLine(x_test, y_test, Point_Position_Background_Width, PRGUI_YELLOW);
        Tft->drawFastHLine(x_test, y_test+Point_Position_Background_Height, Point_Position_Background_Width, PRGUI_YELLOW);
*/        for (uint16_t i=0; i<Regression_Model->Points_num; i++){
            if (i == point_num){
              if (i != Regression_Model->Points_num-1) continue;
            }
            else if (Is_Point_Inside_Square(x_test - Point_Size, x_test + Point_Position_Background_Width + Point_Size, y_test - Point_Size, y_test + Point_Position_Background_Height + Point_Size, Map_X_Point_to_Pixel(Regression_Model->X_points[i]), Map_Y_Point_to_Pixel(Regression_Model->Y_points[i])))
              break;
      /*      else if (Map_X_Point_to_Pixel(X_points[i])+Point_Size > x_test && Map_X_Point_to_Pixel(X_points[i])-Point_Size < x_test + Point_Position_Background_Width){
              if (Map_Y_Point_to_Pixel(Y_points[i])+Point_Size > y_test && Map_Y_Point_to_Pixel(Y_points[i])-Point_Size < y_test + Point_Position_Background_Height){
     //           tft.fillRect(Map_X_Point_to_Pixel(X_points[i]), Map_Y_Point_to_Pixel(Y_points[i]), 5, 5, (uint16_t)random(0xffff));
                break;
              }
            }*/
            if (i == Regression_Model->Points_num-1){
              Point_Position_Square_TL_From_Left = x_test;
              Point_Position_Square_TL_From_Top = y_test;
              goto GoTo1;
            }
          }
        }
        else if (ran_ang>3.14/2 && ran_ang<=3.14){    ///Right Top
          if (x_test < 36) continue;
          if (y_test > Height - 20) continue;
          if (y_test - Point_Position_Background_Height < R_Menu_Top) continue;
          else if (y_test > R_Menu_Top && y_test < R_Menu_Bot-50){
            Serial.println ("B2");
            if (x_test + Point_Position_Background_Height > Width - 50) continue;
          }
          else if (y_test > R_Menu_Bot-50){
            Serial.println ("A1");
            if (x_test + Point_Position_Background_Height > Width - 115) continue;
          }
          for (uint16_t i=0; i<Regression_Model->Points_num; i++){
            if (i == point_num){
              if (i != Regression_Model->Points_num-1) continue;
            }
            else if (Is_Point_Inside_Square(x_test - Point_Size, x_test + Point_Position_Background_Width + Point_Size, y_test - Point_Position_Background_Height - Point_Size, y_test + Point_Size, Map_X_Point_to_Pixel(Regression_Model->X_points[i]), Map_Y_Point_to_Pixel(Regression_Model->Y_points[i])))
              break;
            if (i == Regression_Model->Points_num-1){
              Point_Position_Square_TL_From_Left = x_test;
              Point_Position_Square_TL_From_Top = y_test - Point_Position_Background_Height;
              goto GoTo1;
            }
          }
        }
        else if (ran_ang>3.14 && ran_ang<=3.14*3/2){    ///Left Top
          if (x_test - Point_Position_Background_Width < 36) continue;
          if (y_test > Height - 20) continue;
          if (y_test - Point_Position_Background_Height < R_Menu_Top) continue;
          else if (y_test > R_Menu_Top && y_test < R_Menu_Bot-50){
            if (x_test > Width - 40) continue;
          }
          else if (y_test > R_Menu_Bot-50){
            if (x_test > Width - 115) continue;
          }
          for (uint16_t i=0; i<Regression_Model->Points_num; i++){
            if (i == point_num){
              if (i != Regression_Model->Points_num-1) continue;
            }
            else if (Is_Point_Inside_Square(x_test - Point_Position_Background_Width - Point_Size, x_test + Point_Size, y_test - Point_Position_Background_Height - Point_Size, y_test + Point_Size, Map_X_Point_to_Pixel(Regression_Model->X_points[i]), Map_Y_Point_to_Pixel(Regression_Model->Y_points[i])))
              break;
            if (i == Regression_Model->Points_num-1){
              Point_Position_Square_TL_From_Left = x_test - Point_Position_Background_Width;
              Point_Position_Square_TL_From_Top = y_test - Point_Position_Background_Height;
              goto GoTo1;
            }
          }
        }
        else if (ran_ang>3.14*3/2 && ran_ang<=3.14*2){
          if (x_test - Point_Position_Background_Width < 36) continue;
          if (y_test + Point_Position_Background_Height > Height - 20) continue;
          if (y_test < R_Menu_Top) continue;
          else if (y_test > R_Menu_Top && y_test < R_Menu_Bot-50){
            if (x_test > Width - 40) continue;
          }
          else if (y_test > R_Menu_Bot-50){
            if (x_test > Width - 115) continue;
          }
          for (uint16_t i=0; i<Regression_Model->Points_num; i++){
            if (i == point_num){
              if (i != Regression_Model->Points_num-1) continue;
            }
            else if (Is_Point_Inside_Square(x_test - Point_Position_Background_Width - Point_Size, x_test + Point_Size, y_test - Point_Size, y_test + Point_Position_Background_Height + Point_Size, Map_X_Point_to_Pixel(Regression_Model->X_points[i]), Map_Y_Point_to_Pixel(Regression_Model->Y_points[i])))
              break;
            if (i == Regression_Model->Points_num-1){
              Point_Position_Square_TL_From_Left = x_test - Point_Position_Background_Width;
              Point_Position_Square_TL_From_Top = y_test;
              goto GoTo1;
            }
          }
        }
      }
      Point_Position_Square_TL_From_Left = Width/2;
      Point_Position_Square_TL_From_Top = R_Menu_Top+10;

GoTo1:
    Tft->setCursor(Top_Left_X+Point_Position_Square_TL_From_Left, Top_Left_Y+Point_Position_Square_TL_From_Top);
    Tft->print(temstr);
    Tft->setCursor(Top_Left_X+Point_Position_Square_TL_From_Left, Top_Left_Y+Point_Position_Square_TL_From_Top + 25);
    Tft->print(temstr2);
  }
  void Undraw_Point_Position(){
    Tft->fillRect(Top_Left_X+Point_Position_Square_TL_From_Left, Top_Left_Y+Point_Position_Square_TL_From_Top, Point_Position_Background_Width, Point_Position_Background_Height, PRGUI_BLACK);
    for (uint16_t i = Top_Left_X + Point_Position_Square_TL_From_Left; i <= Top_Left_X + Point_Position_Square_TL_From_Left + Point_Position_Background_Width; i++){
      Draw_Polynomial_At_X_Pixel(i);
    }
    for (uint16_t i=0; i<Regression_Model->Points_num; i++){
      if(Map_X_Point_to_Pixel(Regression_Model->X_points[i]) + Point_Size >= Top_Left_X+Point_Position_Square_TL_From_Left && Map_X_Point_to_Pixel(Regression_Model->X_points[i]) - Point_Size <= Top_Left_X+Point_Position_Square_TL_From_Left + Point_Position_Background_Width)
        if(Map_Y_Point_to_Pixel(Regression_Model->Y_points[i]) + Point_Size >= Top_Left_Y+Point_Position_Square_TL_From_Top && Map_Y_Point_to_Pixel(Regression_Model->Y_points[i]) - Point_Size <= Top_Left_Y+Point_Position_Square_TL_From_Top+Point_Position_Background_Height){
        Draw_Point(i);
      }
    }
  }

  void Cursor_Check_Point_Selection(uint16_t cursor_x, uint16_t cursor_y){
    uint16_t tmp = Get_Closest_Point_num_to_Pix_Or0xffff(cursor_x, cursor_y, Point_Selection_Max_Distance);
    if (tmp != Currently_Selected_Point){
      if(Currently_Selected_Point != 0xffff) Deselect_Point();
      if(tmp < Regression_Model->Points_num) Select_Point(tmp);
    }
  }
public:
  void Set_Point_Selection_Max_Distance(CH_TYPE max_distance){     /// This function sets maximum distance for cursor selection of the point using touch screen
    if (Tft == NULL) return;
    Point_Selection_Max_Distance = max_distance;
  }
  uint16_t Get_Closest_Point_num_to_Pix_Or0xffff(uint16_t x, uint16_t y, float max_distance){     /// This function can be used to manually check for closest point to screen pixel. Returns point index of X_points[], Y_points[]
    if (Tft == NULL) return 0xffff;
    CH_TYPE closest = pow(max_distance, 2); ///   <------------------------ Rewrite it nicely "max CH_TYPE value"
    uint16_t num;
    for (int16_t i=0; i<Regression_Model->Points_num; i++){
        CH_TYPE sq_distance;
        sq_distance = pow( x - Map_X_Point_to_Pixel(Regression_Model->X_points[i]),2);
        sq_distance += pow( y - Map_Y_Point_to_Pixel(Regression_Model->Y_points[i]),2);
        if (sq_distance<closest){
          closest = sq_distance;
          num=i;
        }
    }
    if (closest<pow(max_distance, 2))
      return num;
    else return 0xffff;
  }

  void Fit_Chart_Display_Size(){
    if (Tft == NULL) return;
    if (Regression_Model->Points_num>0){
      X_highest_val = Regression_Model->X_points[0];
      X_lowest_val = Regression_Model->X_points[0];
      Y_highest_val = Regression_Model->Y_points[0];
      Y_lowest_val = Regression_Model->Y_points[0];
      for(int16_t i=0; i<Regression_Model->Points_num; i++){
        if (X_highest_val < Regression_Model->X_points[i])
          X_highest_val = Regression_Model->X_points[i];
        if (X_lowest_val > Regression_Model->X_points[i])
          X_lowest_val = Regression_Model->X_points[i];
        if (Y_highest_val < Regression_Model->Y_points[i])
          Y_highest_val = Regression_Model->Y_points[i];
        if (Y_lowest_val > Regression_Model->Y_points[i])
          Y_lowest_val = Regression_Model->Y_points[i];
      }
      if (X_highest_val == X_lowest_val){
        X_highest_val++;
        X_lowest_val--;
      }
      if (Y_highest_val == Y_lowest_val){
        Y_highest_val++;
        Y_lowest_val--;
      }
    }
  }
  void Select_Point (uint16_t point_num){        /// You can manually select point, selection will be drawn on the screen
    if (Tft == NULL) return;
    Draw_Square_Around_Point(point_num);
    Draw_Point_Position(point_num);
    Currently_Selected_Point = point_num;
  }
  void Deselect_Point(){                         /// To manually deselect point
    if (Tft == NULL) return;
    Undraw_Point(Currently_Selected_Point);
    Draw_Point(Currently_Selected_Point);
    Undraw_Point_Position();
    Currently_Selected_Point = 0xffff;
  }

/*  Draw_Axis(){      /// Was used instead of Draw_X_Axis() and Draw_Y_Axis(), but positions are slightly off due to rounding
 //   Map_X_Pixel_to_Point
    uint16_t interval_x = (Width-42)/(X_axis_descriptions-1);
    uint16_t interval_y = (Height-40)/(Y_axis_descriptions-1);
    Tft->setTextColor(PRGUI_YELLOW);
    Tft->setTextSize(1);
    uint16_t pos = Top_Left_X+30;
    for (uint16_t i=0 ; i<X_axis_descriptions; i++){
      Tft->drawFastVLine(pos, Top_Left_Y+Height-16, 6, PRGUI_YELLOW);
      Tft->setCursor(pos - 11, Top_Left_Y+Height-8);
      Tft->print(String(Map_X_Pixel_to_Point(pos)/*+0.05* /, 1* /).c_str());
      pos += interval_x;
    }
    pos = Top_Left_Y+17;
    for (uint16_t i=0 ; i<Y_axis_descriptions; i++){
  ///    Tft->drawFastHLine(Top_Left_X+16, pos, 5, PRGUI_YELLOW);
      Tft->setCursor(Top_Left_X, pos-3);
      String Y_val_str = String(Map_Y_Pixel_to_Point(pos)/*+0.05* /, 1* /) + '-';
      Tft->print(Y_val_str.c_str());
      pos += interval_y;
    }
  }*/
  void Draw_X_Axis(){
    if (Tft == NULL) return;
    CH_TYPE x_range = X_highest_val - X_lowest_val;
    CH_TYPE x_current = X_lowest_val + x_range/20;
    x_range = x_range - x_range/10;
    int16_t First_X = Map_X_Point_to_Pixel(x_current);
    int16_t X_Pix_Interval = Map_X_Point_to_Pixel(x_current + x_range/(X_axis_descriptions-1)) - First_X;
    Tft->setTextColor(PRGUI_YELLOW);
    Tft->setTextSize(1);
    for (int16_t i = 0; i <X_axis_descriptions; i++){
      int16_t current_pix = First_X + i*X_Pix_Interval;
      char temstr[15];
      dtostren0(Map_X_Pixel_to_Point(current_pix), temstr, 3, 0, 2, 2);
      Tft->drawFastVLine(current_pix, Top_Left_Y+Height-16, 6, PRGUI_YELLOW);
      Tft->setCursor(current_pix - 11, Top_Left_Y+Height-8);
      Tft->print(temstr);
    }

 /*
    CH_TYPE x_interval = x_range/(X_axis_descriptions-1);


    int16_t scienot = Get_Num_Of_Places_To_Right_For_Scientific_Notation(x_interval);
    if (scienot < -2){
      Current_Str = String(x_lowest, 0);
      x_interval = ((int32_t)x_interval)+1;                                 ///  <-------  slow
    }
    else {
      Current_Str =  String(x_lowest, scienot+2);
      x_interval = String(x_interval, scienot+2).toFloat();
    }
    x_current = Current_Str.toFloat();
    Tft->setTextColor(PRGUI_YELLOW);
    Tft->setTextSize(1);
    for (int i = 0; i <X_axis_descriptions; i++){
      Tft->drawFastVLine(Map_X_Point_to_Pixel(x_current), Top_Left_Y+Height-16, 6, PRGUI_YELLOW);
      Tft->setCursor(Map_X_Point_to_Pixel(x_current) - 11, Top_Left_Y+Height-8);
      Tft->print(Current_Str);
      x_current += x_interval;
      Current_Str = String(x_current);
    }*/
  }
  void Draw_Y_Axis(){
    if (Tft == NULL) return;
    CH_TYPE y_range = Y_highest_val - Y_lowest_val;
    CH_TYPE y_current = Y_lowest_val + y_range/20;
    y_range = y_range - y_range/10;
    int16_t First_Y = Map_Y_Point_to_Pixel(y_current);
    int16_t Y_Pix_Interval = Map_Y_Point_to_Pixel(y_current + y_range/(Y_axis_descriptions-1)) - First_Y;
    Tft->setTextColor(PRGUI_YELLOW);
    Tft->setTextSize(1);
    for (int16_t i = 0; i <Y_axis_descriptions; i++){
      int16_t current_pix = First_Y + i*Y_Pix_Interval;
      if (R_Menu_Top > current_pix - Top_Left_Y) break;
      char temstr[15];
      int16_t tmp = dtostren0(Map_Y_Pixel_to_Point(current_pix), temstr, 3, 0, 2, 2);
      temstr[tmp] = '-';
      temstr[tmp+1] = '\0';
      Tft->setCursor(Top_Left_X, current_pix - 3);
      Tft->print(temstr);
    }

  /*  if (Tft == NULL) return;
    CH_TYPE y_range = Y_highest_val - Y_lowest_val;
    CH_TYPE y_lowest = Y_lowest_val + y_range/20;
    y_range = y_range - y_range/10;
    CH_TYPE y_current;
    String Current_Str;
    CH_TYPE y_interval = y_range/(Y_axis_descriptions-1);
    int16_t scienot = Get_Num_Of_Places_To_Right_For_Scientific_Notation(y_interval);
    if (scienot < -2){
      Current_Str = String(y_lowest, 0);
      y_interval = ((int32_t)y_interval)+1;
    }
    else {
      Current_Str =  String(y_lowest, scienot+2);
      y_interval = String(y_interval, scienot+2).toFloat();
    }
    y_current = Current_Str.toFloat();
    Tft->setTextColor(PRGUI_YELLOW);
    Tft->setTextSize(1);
    for (int i = 0; i <Y_axis_descriptions; i++){
      Current_Str += "-";
    //  Tft->drawFastHLine(Map_X_Point_to_Pixel(x_current), Top_Left_Y+Height-16, 6, PRGUI_YELLOW);
      Tft->setCursor(Top_Left_X, Map_Y_Point_to_Pixel(y_current) - 3);
      Tft->print(Current_Str);
      y_current += y_interval;
      Current_Str = String(y_current);
    }*/
  }

  void Draw_Polynomial(){
    if (Tft == NULL) return;
    Calculate_Polynomial_Pixels();
    for(int16_t i=0; i<Width+1; i++){
      Tft->writePixel(Top_Left_X+i, Polynomial_y_pixels[i], PRGUI_GREEN);
    }
  }
  void Draw_Polynomial_Equation(){    /// here is constraint Width should be at least 6
    R_Menu_Top = 1;
    int16_t t1;
    int16_t t2;
    uint16_t tem_wid;
    uint16_t tem_hig;   //not using those finally
    uint16_t max_hig;   //
    uint16_t curr_line_width;
    uint16_t Equation_Lines = 0;

    Tft->setTextColor(PRGUI_CYAN);
    Tft->setTextSize(1);
    Equation_Lines = 0;
    char temstr[12];
    Polynomial_Equation.remove(0);
    if (Regression_Model->Polynomial_coefficients != NULL){
      dtostren0(Regression_Model->Polynomial_coefficients[0], temstr, 4, /*DTOSTR_UPPERCASE | DTOSTR_ALWAYS_SIGN*/0, 3, 3);
      Polynomial_Equation += temstr;//String(Polynomial_coefficients[0], 3);
      if (Regression_Model->Coefficients_Calculate_Num>=1){
        if(Regression_Model->Polynomial_coefficients[1]>=0){
          dtostren0(Regression_Model->Polynomial_coefficients[1], temstr, 4, 0, 3, 3);
          Polynomial_Equation += "+";
          Polynomial_Equation += temstr;
          Polynomial_Equation += "x";
        }
        else{
          dtostren0(Regression_Model->Polynomial_coefficients[1], temstr, 4, 0, 3, 3);
          Polynomial_Equation += temstr;
          Polynomial_Equation += "x";
        }
      }
      Tft->getTextBounds(Polynomial_Equation, 0, 0, &t1, &t2, &curr_line_width, &max_hig);

      for (int16_t i=2; i<=Regression_Model->Coefficients_Calculate_Num; i++){
        if(Regression_Model->Polynomial_coefficients[i]>=0){
          dtostren0(Regression_Model->Polynomial_coefficients[i], temstr, 4, 0, 3, 3);
          String temstr2 = "+";
          temstr2 = temstr2 + temstr + "x^" + String(i);
          Tft->getTextBounds(temstr2, 0, 0, &t1, &t2, &tem_wid, &tem_hig);
          if (curr_line_width + tem_wid > Width-6){
            Tft->setCursor(Top_Left_X+5, Top_Left_Y+Equation_Lines*9);
            Tft->print(Polynomial_Equation.c_str());
            Equation_Lines++;
            Polynomial_Equation.remove(0);
            curr_line_width = tem_wid;
            R_Menu_Top += 9;
          }
          else {
            curr_line_width += tem_wid;
          }
          Polynomial_Equation += temstr2;
        }
        else{
          dtostren0(Regression_Model->Polynomial_coefficients[i], temstr, 4, 0, 3, 3);
          String temstr2 = temstr;
          temstr2 += "x^" + String(i);
          Tft->getTextBounds(temstr2, 0, 0, &t1, &t2, &tem_wid, &tem_hig);
          if (curr_line_width + tem_wid > Width-6){
            Tft->setCursor(Top_Left_X+5, Top_Left_Y+Equation_Lines*9);
            Tft->print(Polynomial_Equation.c_str());
            Equation_Lines++;
            Polynomial_Equation.remove(0);
            curr_line_width = tem_wid;
            R_Menu_Top += 9;
          }
          else {
            curr_line_width += tem_wid;
          }
          Polynomial_Equation += temstr2;
        }
      }
      Tft->setCursor(Top_Left_X+5, Top_Left_Y+Equation_Lines*9);
      Tft->print(Polynomial_Equation.c_str());
      R_Menu_Top += 9;
    }
    else {
      Polynomial_Equation.remove(0);
    }
  }

  void Recalculate_Chart_Scale_Wash_Screen(){
    if (Tft == NULL) return;
    Fit_Chart_Display_Size();
    Tft->fillScreen(PRGUI_BLACK);
  }
  void Draw_Axis(){
    Draw_X_Axis();
    Draw_Y_Axis();
  }
  void Draw_Points(){
    if (Tft == NULL) return;
    for (uint16_t i=0; i<Regression_Model->Points_num; i++){
      uint16_t point_x = Map_X_Point_to_Pixel(Regression_Model->X_points[i]);
      uint16_t point_y = Map_Y_Point_to_Pixel(Regression_Model->Y_points[i]);
      Draw_Point (point_x, point_y);
    }
    if(Currently_Selected_Point != 0xffff) Select_Point(Currently_Selected_Point);
    return;
  }
};


const uint8_t Save_mask[] PROGMEM = { 0, 0, 0, 0, 0, 0, 15, 128, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 120, 192, 0, 0, 0, 0, 224, 0, 0, 0, 0, 0, 224, 0, 0, 0, 0, 0, 224, 0, 248, 192, 113, 252, 224, 3, 252, 224, 115, 254, 248, 3, 14, 96, 103, 142, 124, 2, 14, 112, 231, 15, 63, 0, 6, 48, 199, 7, 15, 128, 126, 48, 207, 255, 7, 193, 254, 57, 207, 255, 1, 231, 254, 25, 142, 0, 0, 231, 14, 25, 134, 0, 0, 231, 14, 15, 135, 0, 192, 231, 30, 15, 7, 6, 255, 199, 254, 15, 3, 254, 255, 195, 254, 6, 1, 254, 127, 1, 230, 0, 0, 112, 0, 0, 0, 0, 0, 0};

const uint16_t Add_Point2_arr[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2049, 2049, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8260, 35121, 47543, 55802, 55835, 57915, 55835, 51705, 41364, 20682, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8260, 55803, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 35121, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28910, 57884, 57916, 57916, 57948, 57948, 58108, 58172, 58076, 57948, 57916, 55803, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33073, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57916, 4098, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33040, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33104, 57916, 57916, 57980, 57980, 58012, 57948, 57948, 57948, 57948, 57948, 57884, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33104, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57884, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30991, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57884, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30959, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57884, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28910, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57884, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28910, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57884, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 26861, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57884, 4098, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 26829, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57884, 4098, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24780, 57884, 57916, 57916, 57948, 57948, 57948, 57948, 58044, 57948, 57916, 57884, 4098, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22699, 57884, 57916, 57916, 57916, 57948, 57948, 57948, 58140, 58140, 57916, 57884, 2050, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24780, 57884, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57916, 57884, 4098, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33040, 57884, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57916, 57916, 12390, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10309, 16520, 16520, 16520, 16520, 16520, 16520, 16520, 16520, 16520, 16520, 14439, 14439, 18569, 30991, 55803, 57884, 57916, 57916, 57916, 57916, 57948, 58172, 58460, 57948, 57948, 57916, 47543, 24780, 22699, 22731, 24780, 24780, 26861, 26861, 28910, 28910, 30991, 30991, 30991, 33040, 33040, 28910, 12390, 0, 0, 2049, 43381, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 58140, 58492, 58364, 57948, 57948, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57883, 14471, 0, 22699, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57916, 41332, 0, 37170, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 58108, 57980, 57948, 57916, 57916, 57916, 57916, 57916, 57948, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 53754, 0, 45462, 57884, 58268, 58652, 58684, 58684, 58684, 58684, 58684, 58684, 58684, 58684, 58684, 58684, 58716, 58460, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 58460, 58300, 58396, 58364, 58492, 58428, 58364, 58268, 58172, 58076, 58012, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57884, 4098, 49592, 57916, 58012, 58236, 58524, 58652, 58716, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58716, 58044, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 58684, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58748, 58716, 58620, 58460, 58236, 58012, 57916, 57884, 6179, 51641, 57884, 57916, 57916, 57916, 57916, 57916, 57948, 58044, 58140, 58300, 58364, 58364, 58332, 58332, 58012, 57916, 57948, 57948, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 58108, 58236, 58236, 58236, 58236, 58236, 58204, 58204, 58204, 58204, 58204, 58172, 58172, 58172, 58044, 57916, 57884, 8260, 49592, 57916, 57916, 57948, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57980, 57980, 57948, 57980, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57980, 57980, 57980, 57980, 57980, 57980, 58012, 57980, 57980, 57948, 57948, 57948, 57980, 57980, 57980, 57884, 6179, 43382, 57884, 57916, 57948, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57980, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57980, 57948, 57980, 57980, 57980, 57980, 57916, 57916, 55835, 2049, 35121, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57916, 57916, 57948, 57948, 57916, 57916, 57916, 57916, 57916, 57916, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 49592, 0, 20650, 55803, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57980, 57916, 57916, 57916, 57916, 57916, 57884, 57916, 57948, 57980, 57980, 57980, 57980, 57980, 57948, 57916, 57916, 57948, 57980, 57980, 57884, 33040, 0, 0, 45462, 55835, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 55804, 57884, 57884, 57884, 57916, 57916, 57916, 57948, 57916, 57916, 57916, 57980, 58044, 57948, 57852, 57884, 57948, 58012, 58012, 58012, 58012, 58012, 58012, 58012, 58012, 58012, 58012, 55963, 45494, 6179, 0, 0, 2049, 18569, 24748, 24844, 22859, 22827, 22827, 20778, 20746, 18730, 18697, 16648, 16616, 14471, 16520, 26829, 53722, 57852, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 58076, 43445, 12390, 8260, 8260, 10309, 10309, 10309, 10309, 10309, 10309, 10309, 10309, 10309, 10309, 10309, 4130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 26829, 57852, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 58044, 16520, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16520, 57852, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57980, 58012, 10309, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 57852, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57980, 58012, 10309, 0, 0, 0, 14755, 21060, 0, 0, 0, 0, 0, 0, 0, 10562, 48522, 29541, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 57852, 57884, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57980, 58012, 10309, 0, 0, 23205, 61166, 61198, 33702, 0, 0, 0, 0, 0, 8417, 56907, 61197, 61196, 40070, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 57852, 57884, 57884, 57884, 57884, 57916, 57916, 57916, 57916, 57980, 58012, 12390, 0, 14755, 61134, 61198, 61197, 61197, 35879, 0, 0, 0, 4257, 52715, 61197, 61196, 61195, 61194, 27396, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16488, 57852, 57884, 57884, 57884, 57884, 57916, 57916, 57916, 57916, 57948, 58012, 12390, 0, 42153, 61165, 61196, 61196, 61197, 61196, 40039, 32, 2112, 48522, 61197, 61196, 61195, 61194, 61161, 25283, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16488, 55804, 57884, 57884, 57884, 57884, 57884, 57916, 57884, 57884, 57948, 58012, 14439, 0, 12674, 56939, 61164, 61164, 61196, 61196, 61196, 46409, 48489, 61197, 61196, 61195, 61194, 61161, 35845, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14440, 55804, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57948, 58012, 14439, 0, 0, 10529, 54826, 61164, 61163, 61195, 61196, 61196, 61196, 61196, 61195, 61162, 61161, 40037, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14440, 55804, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57948, 58012, 16519, 0, 0, 0, 6369, 52681, 61163, 61163, 61195, 61195, 61195, 61195, 61162, 61161, 42182, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14440, 55804, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57916, 58012, 16520, 0, 0, 0, 0, 4224, 50601, 61164, 61163, 61195, 61163, 61162, 61162, 50631, 2112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 55804, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57916, 58012, 16520, 0, 0, 0, 0, 0, 33734, 61164, 61163, 61163, 61162, 61162, 61162, 54889, 10529, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 55804, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57948, 58012, 16520, 0, 0, 0, 0, 21092, 61164, 61164, 61163, 61162, 61162, 61162, 61162, 61194, 56937, 12674, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 55804, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57948, 58012, 18569, 0, 0, 0, 16899, 61132, 61165, 61163, 61162, 61162, 61162, 61162, 61162, 61162, 61194, 59049, 16898, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 55804, 57852, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57948, 58012, 18601, 0, 0, 14754, 59052, 61165, 61163, 61162, 61162, 56968, 42181, 61161, 61162, 61162, 61162, 61162, 59082, 21123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14439, 55804, 57852, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57916, 57980, 18569, 0, 10529, 56972, 61165, 61163, 61162, 61162, 59049, 14754, 0, 29507, 61161, 61162, 61162, 61162, 61194, 59081, 6336, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6179, 55803, 55804, 55804, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57851, 10309, 0, 29509, 61165, 61164, 61162, 61162, 59081, 19010, 0, 0, 0, 25315, 59081, 61162, 61162, 61162, 52744, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 26829, 53723, 55803, 55804, 55804, 55804, 55804, 55804, 55804, 53722, 28910, 0, 0, 2144, 50600, 61162, 61162, 61161, 25283, 0, 0, 0, 0, 0, 21090, 59081, 61161, 56968, 10593, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4130, 16520, 24780, 28910, 28911, 26830, 24748, 14440, 4098, 0, 0, 0, 0, 4224, 44326, 61161, 29540, 0, 0, 0, 0, 0, 0, 0, 16866, 40037, 10529, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8449, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t Add_Point1_mask[] PROGMEM = { 0, 0, 3, 128, 0, 0, 0, 0, 63, 248, 0, 0, 0, 0, 127, 248, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 127, 255, 255, 255, 255, 252, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 248, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 127, 252, 96, 56, 0, 0, 127, 252, 240, 126, 0, 0, 127, 253, 248, 254, 0, 0, 127, 253, 255, 254, 0, 0, 127, 253, 255, 252, 0, 0, 127, 252, 255, 248, 0, 0, 127, 252, 127, 248, 0, 0, 127, 252, 63, 240, 0, 0, 127, 252, 31, 240, 0, 0, 127, 252, 63, 248, 0, 0, 127, 252, 127, 252, 0, 0, 127, 252, 255, 254, 0, 0, 127, 253, 254, 255, 0, 0, 127, 253, 252, 127, 0, 0, 63, 249, 248, 62, 0, 0, 31, 240, 240, 28, 0, 0, 0, 0, 32, 0};
const uint16_t Add_Point1_arr[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2113, 2145, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8516, 33904, 48662, 55033, 57146, 57178, 57146, 52920, 42355, 21194, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8516, 57114, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 35984, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29645, 57178, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 57114, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33904, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33904, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33871, 57179, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31791, 57179, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31759, 57179, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31726, 57179, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29646, 57179, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59227, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29613, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59227, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27533, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59227, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27500, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59227, 59227, 4226, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25420, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59227, 59227, 4225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23307, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59227, 59227, 2177, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25387, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59227, 59227, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33872, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 12710, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 10597, 16936, 16968, 16968, 16968, 16968, 16968, 16936, 16968, 16936, 16935, 14855, 14855, 19048, 31726, 57146, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59227, 59227, 59227, 48662, 25420, 23274, 23307, 25387, 25420, 27500, 27533, 29613, 29645, 29678, 31726, 31758, 31791, 33839, 29613, 12710, 0, 0, 2113, 44404, 57178, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 57146, 14823, 0, 23274, 57178, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59227, 59259, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59227, 42291, 0, 38097, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59227, 52953, 0, 46581, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59227, 57178, 4225, 50775, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 6371, 52888, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 57179, 8484, 48695, 57178, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 57178, 6338, 44469, 57178, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 57146, 2113, 35984, 57178, 57178, 57178, 57178, 57179, 57179, 57178, 57179, 57179, 57179, 57179, 57179, 59227, 57179, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 50775, 0, 21194, 57146, 57178, 57178, 57178, 57178, 57178, 57178, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57178, 57178, 31791, 0, 32, 46549, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57147, 57146, 57146, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 57179, 57179, 57179, 57179, 57179, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57146, 46581, 6338, 0, 0, 2113, 19048, 25387, 25387, 23307, 23274, 23274, 21194, 21161, 19081, 19048, 16968, 16935, 14855, 16936, 27532, 55001, 57178, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 44436, 12742, 8516, 8516, 8516, 10596, 10597, 10597, 10597, 10597, 10597, 10597, 10597, 10597, 10564, 4290, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27533, 57179, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 16935, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16936, 57179, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 10596, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57179, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 10564, 0, 0, 0, 10692, 14982, 0, 0, 0, 0, 0, 0, 0, 6499, 36367, 19368, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57179, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 10597, 0, 0, 17160, 49013, 49045, 23562, 0, 0, 0, 0, 0, 4354, 40658, 46964, 44915, 25835, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57179, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 12709, 0, 10725, 46965, 46964, 46964, 46964, 25739, 0, 0, 0, 2209, 38513, 46964, 44915, 42866, 40818, 15175, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57178, 57179, 57178, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 12742, 0, 29997, 46964, 44916, 44916, 46964, 46964, 27852, 32, 96, 34287, 46964, 44915, 42866, 40785, 36688, 13062, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16903, 57146, 57178, 57178, 57179, 59227, 59227, 59227, 59227, 59227, 59227, 57179, 14822, 0, 8611, 38642, 44915, 44915, 44915, 44915, 44915, 34254, 34287, 46964, 44915, 42866, 40785, 36689, 17480, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57146, 57178, 57178, 57179, 57178, 59227, 59227, 59227, 59227, 59227, 57179, 14855, 0, 0, 6467, 36528, 42867, 42867, 44915, 44915, 44915, 46964, 44915, 42866, 40785, 36689, 19658, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57146, 57178, 57178, 57179, 57179, 59227, 59227, 59227, 59227, 59227, 57179, 14855, 0, 0, 0, 4322, 32335, 40818, 42866, 42867, 44915, 44915, 42866, 40786, 36689, 21803, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57146, 57178, 57178, 57178, 57179, 59227, 59227, 59227, 59227, 59227, 57179, 16935, 0, 0, 0, 0, 2209, 32270, 42867, 42866, 42866, 42866, 40818, 40785, 28205, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57146, 57178, 57178, 57178, 57178, 57179, 59227, 59227, 59227, 57179, 57179, 16936, 0, 0, 0, 0, 0, 21546, 42867, 42866, 42866, 40786, 40786, 40785, 34479, 4418, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57146, 57178, 57178, 57178, 57178, 57179, 57179, 57179, 59227, 57179, 57179, 16968, 0, 0, 0, 0, 12966, 44915, 44915, 42866, 40786, 40785, 40785, 40785, 40818, 36560, 6563, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57146, 57178, 57178, 57178, 57178, 57179, 57179, 57179, 57179, 57179, 57179, 19048, 0, 0, 0, 10789, 44883, 44916, 42866, 40785, 38737, 36689, 38737, 40785, 40786, 40818, 36624, 8740, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14855, 57146, 57178, 57178, 57178, 57178, 57179, 57179, 57179, 57179, 57179, 57179, 19081, 0, 0, 8644, 42803, 44916, 42867, 40786, 36689, 30447, 17706, 34640, 38737, 38737, 40785, 40785, 38705, 12965, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14822, 57146, 57178, 57178, 57178, 57178, 57178, 57179, 57179, 57179, 57179, 57178, 19048, 0, 6467, 42770, 44916, 42867, 40786, 38737, 32527, 6595, 0, 11142, 32592, 36689, 38737, 40785, 40785, 36657, 2241, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6371, 57146, 57146, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57146, 10564, 0, 19369, 44915, 42867, 40786, 38737, 32560, 8804, 0, 0, 0, 11046, 32560, 36689, 38737, 38737, 30286, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27533, 55034, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 55033, 29613, 0, 0, 96, 30222, 40817, 38737, 32592, 11014, 0, 0, 0, 0, 0, 8869, 30511, 36688, 32463, 4450, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4258, 16968, 25420, 29613, 31726, 27533, 25387, 14855, 4226, 0, 0, 0, 0, 2209, 21867, 32592, 13223, 0, 0, 0, 0, 0, 0, 0, 6660, 17609, 4418, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2305, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint16_t Subtract_Point_arr[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10564, 21161, 21194, 21194, 21194, 21194, 21194, 21194, 23274, 23274, 23274, 23307, 23307, 25387, 25387, 25420, 25420, 25452, 27500, 27500, 27532, 27533, 27533, 27565, 29613, 29613, 29645, 29646, 29678, 31726, 31758, 31759, 31791, 33839, 33871, 33871, 33904, 35984, 36017, 35952, 23307, 32, 0, 0, 0, 0, 0, 23307, 57146, 59226, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 31726, 0, 0, 0, 0, 0, 52888, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 55001, 0, 0, 0, 0, 6371, 57146, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 6403, 0, 0, 0, 14790, 57178, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 12742, 0, 0, 0, 16903, 57178, 59227, 59227, 59227, 59227, 59227, 59227, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59259, 59227, 59227, 16903, 0, 0, 0, 16903, 57146, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 14823, 0, 0, 0, 12710, 57146, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 59227, 57179, 12677, 0, 0, 0, 4258, 57146, 57178, 57178, 57178, 57178, 57178, 57178, 57179, 57178, 57178, 57179, 57178, 57179, 57178, 57178, 57178, 57179, 57178, 57178, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 57179, 59227, 57146, 4226, 0, 0, 0, 0, 50775, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 46549, 0, 0, 0, 0, 0, 21194, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 52920, 14823, 0, 0, 0, 0, 0, 0, 10629, 23274, 23339, 23307, 23274, 21194, 21194, 21161, 21161, 19081, 19081, 19081, 19048, 19048, 16968, 16968, 16968, 16936, 16935, 14855, 14855, 14855, 14823, 14823, 14822, 12742, 12742, 12742, 12710, 12709, 10629, 10629, 10629, 10597, 10597, 10597, 10597, 10597, 10597, 8484, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10789, 2177, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2177, 32110, 38481, 4354, 0, 0, 0, 0, 0, 0, 25707, 46964, 40658, 8611, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 34287, 46964, 46964, 40658, 6499, 0, 0, 0, 0, 21449, 46964, 46964, 44915, 38673, 4418, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21513, 42866, 42867, 44915, 46964, 42738, 8612, 0, 0, 17159, 44916, 44916, 44915, 42866, 38737, 19497, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15336, 36688, 40785, 42866, 44915, 46964, 42770, 10821, 12966, 44915, 44915, 42867, 42866, 38737, 30350, 2273, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15432, 34640, 38737, 42866, 42867, 44915, 46964, 46964, 44915, 42866, 40818, 38737, 30415, 4418, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13191, 32592, 38737, 40786, 42866, 44915, 44915, 42866, 40786, 38737, 32495, 4483, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8933, 32560, 38737, 40818, 42866, 42866, 40818, 40785, 34608, 6627, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15336, 38737, 40785, 40786, 40786, 40785, 40785, 28076, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2177, 32334, 40786, 40785, 40785, 40785, 40785, 40785, 40818, 23754, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 30190, 40818, 40786, 40785, 38737, 36689, 38737, 40785, 40818, 40818, 25932, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64, 28077, 40818, 40818, 40785, 38737, 34640, 30543, 34640, 38737, 40785, 40785, 40786, 30157, 2209, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 25932, 40818, 40786, 40785, 38737, 34640, 17545, 2305, 22028, 34640, 36689, 38737, 40785, 40785, 30254, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15207, 40818, 40786, 38737, 38737, 34640, 19722, 32, 0, 129, 19883, 32592, 36688, 38737, 40785, 36656, 2209, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4515, 34608, 38737, 38737, 34640, 21899, 64, 0, 0, 0, 96, 17770, 32592, 36688, 36688, 15239, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6756, 30479, 34640, 24044, 96, 0, 0, 0, 0, 0, 32, 15593, 28463, 13255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2402, 13287, 2209, 0, 0, 0, 0, 0, 0, 0, 0, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t Subtract_Point_mask[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 255, 255, 255, 255, 252, 31, 255, 255, 255, 255, 252, 31, 255, 255, 255, 255, 252, 63, 255, 255, 255, 255, 254, 63, 255, 255, 255, 255, 254, 63, 255, 255, 255, 255, 254, 63, 255, 255, 255, 255, 254, 63, 255, 255, 255, 255, 254, 63, 255, 255, 255, 255, 254, 31, 255, 255, 255, 255, 252, 31, 255, 255, 255, 255, 252, 15, 255, 255, 255, 255, 248, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 48, 0, 0, 0, 1, 224, 120, 0, 0, 0, 3, 240, 252, 0, 0, 0, 3, 249, 252, 0, 0, 0, 3, 255, 252, 0, 0, 0, 1, 255, 248, 0, 0, 0, 0, 255, 240, 0, 0, 0, 0, 127, 224, 0, 0, 0, 0, 63, 192, 0, 0, 0, 0, 127, 240, 0, 0, 0, 0, 255, 248, 0, 0, 0, 1, 255, 252, 0, 0, 0, 3, 255, 254, 0, 0, 0, 3, 253, 254, 0, 0, 0, 3, 248, 252, 0, 0, 0, 1, 240, 120, 0, 0, 0, 0, 224, 16, 0, 0, 0, 0, 0, 0};
const uint16_t Subtract_Point_arr2[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10309, 20650, 20650, 20650, 20650, 20650, 20650, 20650, 22699, 22731, 22731, 22731, 22731, 24780, 24780, 24780, 24780, 24780, 26861, 26861, 26861, 26861, 26861, 26861, 28910, 28910, 28942, 28942, 30991, 30991, 30991, 30991, 30991, 33040, 33072, 33072, 35121, 35121, 35121, 35121, 22731, 0, 0, 0, 0, 0, 0, 22731, 55835, 57884, 57884, 57884, 57916, 57884, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 30991, 0, 0, 0, 0, 0, 51641, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 53754, 0, 0, 0, 0, 6179, 57884, 57916, 58076, 58076, 58076, 58108, 58108, 58108, 58108, 58108, 58108, 58140, 58140, 58140, 58140, 58140, 58172, 58172, 58172, 58172, 58172, 58172, 58204, 58204, 58204, 58204, 58204, 58236, 58236, 58236, 58236, 58236, 58172, 58076, 58012, 57948, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 6179, 0, 0, 0, 14439, 57884, 57916, 58556, 58716, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58780, 58684, 58652, 58556, 58268, 58076, 57916, 57916, 12390, 0, 0, 0, 16520, 57884, 57916, 57916, 57948, 58076, 58108, 58204, 58268, 58364, 58428, 58524, 58620, 58716, 58780, 58748, 58748, 58748, 58748, 58748, 58716, 58716, 58716, 58716, 58716, 58684, 58684, 58684, 58684, 58684, 58684, 58652, 58652, 58652, 58652, 58652, 58620, 58620, 58620, 58620, 58620, 58460, 57916, 57916, 16520, 0, 0, 0, 16520, 57884, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57980, 57948, 57948, 57948, 57916, 57916, 14439, 0, 0, 0, 12390, 55836, 57916, 57948, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57916, 57948, 57948, 57948, 57948, 57948, 57948, 57948, 57980, 57980, 57980, 57980, 57980, 57948, 57980, 57980, 57980, 57948, 57980, 57980, 57980, 57980, 57980, 57980, 57980, 57980, 57980, 57948, 57980, 57980, 57948, 57916, 57916, 12390, 0, 0, 0, 4130, 55803, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 55835, 4098, 0, 0, 0, 0, 49560, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57916, 57916, 57916, 57916, 57916, 57948, 57916, 57884, 57916, 57948, 57948, 57948, 57948, 57916, 57884, 45462, 0, 0, 0, 0, 0, 20650, 55771, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55803, 55931, 55963, 55963, 55963, 55963, 55963, 55963, 55963, 55931, 55963, 55963, 55963, 55963, 55931, 51705, 14439, 0, 0, 0, 0, 0, 0, 10310, 22731, 22828, 22827, 22827, 20778, 20778, 20746, 20778, 18698, 18697, 18697, 18697, 18697, 16648, 16616, 16616, 16648, 16616, 16616, 14567, 14567, 14535, 14471, 14439, 14439, 12390, 12390, 12390, 12390, 12358, 10309, 10309, 10309, 10309, 10309, 10309, 10309, 10309, 8292, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16899, 4224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4224, 44265, 50602, 8417, 0, 0, 0, 0, 0, 0, 35847, 61165, 54859, 12674, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2112, 48522, 61197, 61197, 56907, 10530, 0, 0, 0, 0, 31589, 61165, 61197, 61196, 57002, 10529, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33701, 61163, 61195, 61196, 61197, 56971, 12674, 0, 0, 25284, 61165, 61197, 61195, 61195, 61162, 33765, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31652, 61161, 61162, 61195, 61196, 61197, 59051, 18947, 21091, 61164, 61196, 61195, 61195, 61162, 54824, 6369, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 35812, 61161, 61162, 61163, 61195, 61196, 61196, 61196, 61196, 61195, 61194, 61162, 56936, 10529, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29508, 61161, 61162, 61162, 61195, 61196, 61195, 61195, 61162, 61162, 56968, 12641, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23203, 59081, 61162, 61162, 61195, 61195, 61194, 61162, 59081, 14786, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31652, 61162, 61162, 61162, 61162, 61162, 61162, 46439, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4224, 52712, 61162, 61162, 61162, 61162, 61162, 61162, 61194, 40038, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2144, 48520, 61163, 61162, 61162, 61162, 61161, 61162, 61162, 61194, 61194, 44295, 2112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2112, 46407, 61163, 61162, 61162, 61162, 61161, 61161, 61161, 61162, 61162, 61162, 61194, 48519, 4224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 44263, 61163, 61162, 61162, 61162, 61161, 37925, 8416, 50599, 61161, 61161, 61162, 61162, 61194, 50632, 2112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27428, 61162, 61162, 61162, 61162, 61161, 42149, 0, 0, 4224, 46438, 61161, 61161, 61162, 61162, 59081, 4256, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12673, 59081, 61162, 61161, 61161, 46374, 2112, 0, 0, 0, 2112, 44294, 61161, 61161, 61161, 29540, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19010, 59048, 61161, 48551, 2144, 0, 0, 0, 0, 0, 32, 40069, 59080, 31620, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10561, 31652, 4256, 0, 0, 0, 0, 0, 0, 0, 0, 2144, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint16_t Calculate_B_arr2[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6337, 37990, 4192, 0, 0, 0, 0, 0, 29572, 52745, 8449, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 46407, 61194, 50600, 6304, 0, 0, 0, 25283, 61196, 61195, 25348, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12674, 56937, 61195, 54857, 16866, 32, 27428, 61163, 61196, 33734, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8449, 54858, 61196, 61197, 58960, 60947, 60917, 56404, 24844, 14471, 2049, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12642, 60978, 60729, 58619, 58619, 58555, 57916, 57916, 57916, 55835, 41364, 20650, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2081, 41556, 58587, 58619, 58619, 58713, 58647, 45558, 53786, 57948, 57916, 57916, 57916, 51705, 22826, 0, 0, 0, 0, 0, 0, 32, 16866, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2049, 45621, 58108, 58523, 60822, 61103, 61197, 61198, 27430, 0, 8260, 22763, 43413, 57884, 57980, 58555, 45973, 8323, 0, 0, 0, 0, 35877, 61195, 31621, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 35281, 58012, 57980, 60915, 59051, 37958, 50600, 61196, 61197, 27429, 0, 0, 0, 10309, 43763, 58587, 58619, 56441, 16679, 0, 0, 31653, 61195, 61162, 23203, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14535, 58171, 58076, 60693, 54858, 10529, 0, 32, 35845, 61194, 61196, 35814, 0, 0, 0, 0, 35659, 58618, 58619, 58554, 25129, 40071, 61195, 61195, 25348, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45590, 58331, 58712, 56907, 8481, 0, 0, 0, 0, 29540, 61162, 48520, 0, 0, 0, 0, 0, 33672, 58681, 58619, 58650, 61103, 61196, 35845, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16520, 57916, 57916, 50350, 14722, 0, 0, 0, 0, 0, 0, 21091, 4224, 0, 0, 0, 0, 0, 0, 48459, 58649, 58619, 60761, 59052, 2144, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43413, 57916, 45462, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 42184, 61073, 58619, 58587, 58867, 8417, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12390, 57884, 57916, 18601, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14754, 61163, 61164, 60885, 58587, 58650, 50603, 4256, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37202, 57916, 49624, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8449, 56938, 61195, 52714, 21061, 56441, 58619, 60885, 50665, 6369, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12641, 32, 0, 10309, 55835, 57884, 24780, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8417, 54825, 61194, 50600, 4224, 0, 31150, 58012, 58075, 61133, 54857, 0, 0, 8449, 0, 0, 0, 0, 0, 0, 4224, 27428, 32, 0, 0, 0, 32, 42182, 37958, 0, 0, 0, 0, 0, 16834, 59082, 42184, 2081, 47543, 57884, 51673, 2049, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23203, 61162, 52712, 4224, 0, 0, 6211, 55867, 57916, 60756, 44296, 0, 18978, 59081, 35845, 0, 0, 0, 0, 2112, 46374, 61162, 35878, 0, 0, 0, 14786, 61162, 61162, 40102, 2112, 0, 0, 12674, 56969, 61164, 48395, 43445, 57884, 57884, 16520, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21091, 8449, 0, 0, 0, 0, 39283, 57916, 49719, 32, 0, 27396, 61162, 61162, 40070, 2112, 0, 2112, 42214, 61162, 59018, 12674, 0, 0, 0, 0, 21091, 59083, 61163, 50600, 18979, 25316, 59050, 61165, 58802, 51768, 57884, 57884, 26861, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18601, 57884, 57884, 4130, 0, 0, 21092, 61131, 61163, 52745, 31653, 50600, 61163, 61132, 16835, 0, 0, 0, 0, 0, 0, 18947, 61132, 61164, 61164, 61102, 60947, 58265, 57916, 57884, 55835, 24812, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2049, 53754, 57884, 14471, 0, 24780, 28974, 45904, 60883, 60978, 60978, 60978, 60978, 45967, 22795, 24812, 24812, 26861, 28942, 33072, 35185, 39315, 54199, 58713, 58650, 58587, 58587, 58011, 57884, 45494, 12390, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20651, 30991, 2049, 12390, 57884, 57884, 57884, 58331, 58587, 58587, 58587, 58427, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 57884, 58075, 58587, 58587, 58587, 58650, 48020, 14503, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4130, 37202, 45462, 49656, 58425, 58649, 58649, 58649, 58554, 55962, 51737, 49656, 49592, 47543, 45462, 43381, 39315, 39378, 58581, 60915, 60977, 61070, 61164, 59051, 14786, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21092, 61164, 61163, 61131, 61131, 61164, 59020, 14754, 0, 0, 0, 0, 0, 0, 42152, 61164, 59082, 29540, 19011, 50600, 61162, 59050, 18979, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14754, 59050, 61162, 44294, 6337, 10529, 48519, 61162, 59051, 18979, 0, 0, 0, 0, 37958, 61163, 59049, 18946, 0, 0, 2112, 42182, 61162, 61162, 21059, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12641, 56969, 61161, 42182, 32, 0, 0, 2112, 42182, 61162, 61162, 8417, 0, 0, 6337, 56969, 59081, 19010, 0, 0, 0, 0, 32, 40037, 54856, 8417, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14754, 59049, 46406, 2112, 0, 0, 0, 0, 32, 40069, 29540, 0, 0, 0, 0, 10561, 23171, 0, 0, 0, 0, 0, 0, 0, 4224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8481, 4224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t Calculate_B_mask1[] PROGMEM = { 0, 0, 0, 1, 193, 192, 0, 0, 0, 0, 1, 227, 192, 0, 0, 0, 0, 1, 255, 128, 0, 0, 0, 0, 0, 255, 224, 0, 0, 0, 0, 0, 127, 252, 0, 0, 0, 0, 0, 255, 254, 6, 0, 0, 0, 1, 255, 127, 135, 0, 0, 0, 7, 255, 143, 207, 0, 0, 0, 7, 247, 195, 254, 0, 0, 0, 15, 225, 193, 252, 0, 0, 0, 31, 192, 192, 252, 0, 0, 0, 62, 0, 0, 252, 0, 0, 0, 60, 0, 1, 254, 0, 0, 0, 120, 0, 3, 255, 0, 0, 0, 240, 0, 7, 223, 32, 113, 193, 240, 0, 7, 159, 112, 241, 243, 224, 0, 3, 15, 125, 240, 255, 192, 0, 0, 15, 127, 224, 127, 128, 0, 0, 15, 255, 192, 63, 0, 0, 0, 7, 255, 255, 255, 0, 0, 0, 0, 127, 255, 255, 128, 0, 0, 0, 31, 255, 255, 192, 0, 0, 0, 63, 255, 243, 224, 0, 0, 0, 124, 249, 225, 224, 0, 0, 0, 120, 112, 192, 192, 0, 0, 0, 48, 0, 0, 0, 0, 0, 0};
const uint8_t Calculate_B_mask2[] PROGMEM = { 0, 0, 0, 1, 193, 192, 0, 0, 0, 0, 1, 227, 192, 0, 0, 0, 0, 1, 255, 128, 0, 0, 0, 0, 0, 255, 224, 0, 0, 0, 0, 0, 127, 248, 0, 0, 0, 0, 0, 255, 254, 6, 0, 0, 0, 1, 255, 127, 135, 0, 0, 0, 1, 255, 143, 207, 0, 0, 0, 3, 247, 195, 254, 0, 0, 0, 3, 225, 193, 252, 0, 0, 0, 7, 192, 192, 252, 0, 0, 0, 7, 0, 0, 252, 0, 0, 0, 15, 0, 1, 254, 0, 0, 0, 14, 0, 3, 255, 0, 0, 0, 222, 0, 7, 223, 32, 113, 193, 254, 0, 7, 159, 112, 241, 243, 252, 0, 3, 15, 125, 240, 255, 248, 0, 0, 15, 63, 224, 127, 240, 0, 0, 15, 127, 255, 255, 224, 0, 0, 7, 255, 255, 255, 128, 0, 0, 0, 255, 255, 255, 128, 0, 0, 0, 31, 224, 127, 192, 0, 0, 0, 63, 240, 243, 224, 0, 0, 0, 124, 249, 225, 224, 0, 0, 0, 120, 112, 192, 64, 0, 0, 0, 48, 0, 0, 0, 0, 0, 0};

const uint16_t Calculate_B_arr[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 225, 7334, 128, 0, 0, 0, 0, 0, 5029, 11881, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9639, 16235, 11784, 193, 0, 0, 0, 4868, 24396, 20331, 6948, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2466, 13993, 18283, 13961, 2562, 32, 7012, 16202, 24427, 11269, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2337, 18057, 20331, 20331, 30474, 38698, 44842, 44681, 27299, 14658, 2080, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6594, 42794, 57098, 61226, 61226, 63274, 65258, 65258, 65226, 63049, 46182, 23011, 2080, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23042, 58888, 63242, 61194, 59081, 52937, 52873, 52390, 60776, 64969, 65097, 65226, 65258, 58824, 25187, 0, 0, 0, 0, 0, 0, 32, 2562, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 39813, 65194, 65226, 61065, 42729, 26442, 20331, 30572, 11108, 0, 10433, 27171, 50150, 65001, 65226, 63274, 46407, 8417, 0, 0, 0, 0, 7270, 18283, 7109, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2080, 48198, 65194, 65194, 56905, 32555, 16138, 9350, 11784, 20299, 26475, 11108, 0, 0, 0, 12545, 44166, 59081, 61226, 57001, 16866, 0, 0, 7141, 16203, 14154, 2787, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 46086, 65193, 65129, 52873, 24426, 13993, 2369, 0, 32, 7237, 14154, 20299, 13349, 0, 0, 0, 0, 21509, 57001, 61226, 59114, 21252, 9414, 16203, 14155, 4900, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 35556, 65162, 65129, 48648, 24426, 14025, 289, 0, 0, 0, 0, 5028, 14154, 11720, 0, 0, 0, 0, 0, 13317, 52905, 61194, 59178, 28490, 18283, 7238, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18786, 65097, 65161, 41829, 2241, 11655, 2466, 0, 0, 0, 0, 0, 0, 2691, 128, 0, 0, 0, 0, 0, 0, 17863, 54985, 61226, 55082, 18218, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4160, 58728, 65161, 54439, 2080, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13575, 32554, 59081, 61226, 38666, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43973, 65129, 62921, 12513, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2530, 16202, 20330, 44777, 61194, 59146, 19976, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27171, 65097, 65097, 29251, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 289, 14025, 16202, 15944, 10883, 56904, 61194, 44842, 11817, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12706, 62952, 65161, 48134, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 257, 11913, 14154, 11752, 128, 0, 33572, 65193, 63178, 26442, 11945, 0, 0, 289, 0, 0, 0, 0, 0, 0, 128, 4964, 32, 0, 0, 0, 32, 7431, 7302, 0, 0, 0, 0, 0, 4578, 52905, 63177, 58728, 6240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2755, 14154, 11848, 160, 0, 0, 6272, 62952, 65194, 44810, 13639, 0, 2626, 12074, 5157, 0, 0, 0, 0, 64, 7559, 12106, 7269, 0, 0, 0, 2530, 14154, 14154, 7398, 64, 0, 0, 2466, 44680, 61161, 59049, 14722, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2691, 257, 0, 0, 0, 0, 46021, 65161, 54632, 32, 0, 4900, 16202, 14154, 7398, 64, 0, 64, 7463, 12106, 16105, 2466, 0, 0, 0, 0, 4739, 14122, 14154, 11784, 4707, 4868, 42665, 61161, 59081, 25283, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22914, 65065, 65129, 6272, 0, 4192, 10979, 24362, 16202, 13896, 7141, 9736, 14154, 16170, 4578, 0, 0, 0, 0, 0, 0, 4642, 18218, 18250, 20298, 46857, 61161, 59081, 33700, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4128, 62792, 65129, 18850, 8320, 65032, 63016, 56937, 46857, 32554, 20298, 18250, 20298, 9028, 0, 0, 0, 0, 0, 0, 0, 0, 13220, 38698, 55017, 61161, 59081, 35845, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33347, 48070, 8353, 10433, 62728, 64936, 65065, 61097, 61161, 59113, 50953, 44809, 23171, 12609, 6272, 2080, 2080, 6240, 12577, 25155, 44069, 58984, 61161, 59113, 57001, 36617, 6948, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4160, 20898, 37636, 52776, 56968, 59049, 59081, 61161, 63113, 65129, 65097, 65065, 65033, 65065, 65097, 65097, 65097, 61033, 56968, 42697, 22314, 16202, 14090, 2530, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6819, 18250, 20266, 28393, 38569, 50824, 60904, 64904, 64872, 64872, 64872, 64840, 64808, 62696, 54728, 38569, 20233, 5028, 2659, 7688, 12106, 14090, 2626, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2498, 14089, 12106, 7495, 225, 321, 9671, 18250, 32554, 23396, 18722, 18690, 14561, 8353, 17574, 22346, 12042, 2594, 0, 0, 96, 5414, 10058, 12074, 2659, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 386, 9961, 10058, 5414, 32, 0, 0, 64, 5414, 14154, 22346, 257, 0, 0, 2273, 16105, 10026, 611, 0, 0, 0, 0, 32, 3238, 7817, 257, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 450, 9993, 5543, 64, 0, 0, 0, 0, 32, 5318, 2948, 0, 0, 0, 0, 353, 2723, 0, 0, 0, 0, 0, 0, 32, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 289, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t Arrow_Right_mask[] PROGMEM = { 0, 56, 0, 0, 62, 0, 0, 63, 128, 0, 51, 224, 127, 240, 248, 255, 240, 30, 192, 48, 7, 128, 48, 15, 192, 48, 30, 255, 240, 120, 127, 240, 240, 0, 19, 192, 0, 23, 128, 0, 30, 0, 0, 28, 0};
const uint8_t Arrow_Left_mask[] PROGMEM = { 0, 28, 0, 0, 124, 0, 1, 252, 0, 7, 204, 0, 31, 15, 254, 120, 15, 255, 224, 12, 3, 240, 12, 1, 120, 12, 3, 30, 15, 255, 15, 15, 254, 3, 200, 0, 1, 232, 0, 0, 120, 0, 0, 56, 0};
const uint8_t Arrow_Select_mask[] PROGMEM = { 0, 7, 128, 0, 0, 7, 224, 0, 0, 15, 248, 0, 63, 140, 126, 0, 63, 252, 31, 128, 113, 252, 7, 224, 96, 60, 1, 248, 224, 12, 0, 126, 192, 12, 0, 31, 192, 12, 0, 7, 224, 12, 0, 31, 126, 12, 0, 62, 63, 204, 0, 248, 15, 254, 3, 240, 0, 254, 7, 192, 0, 14, 31, 0, 0, 14, 126, 0, 0, 6, 248, 0, 0, 7, 224, 0, 0, 7, 128, 0};

const uint8_t Zero_mask[] PROGMEM = { 1, 255, 255, 192, 15, 255, 255, 248, 63, 255, 255, 252, 62, 0, 0, 62, 112, 7, 240, 14, 240, 31, 252, 15, 240, 63, 254, 15, 224, 127, 254, 7, 224, 252, 63, 7, 224, 248, 31, 7, 225, 240, 15, 135, 225, 240, 15, 135, 225, 240, 15, 135, 225, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 227, 224, 7, 199, 225, 224, 7, 199, 225, 240, 7, 135, 225, 240, 15, 135, 225, 240, 15, 135, 240, 248, 31, 7, 240, 252, 63, 7, 240, 127, 254, 7, 240, 127, 254, 7, 112, 63, 252, 15, 112, 15, 240, 15, 120, 3, 192, 30, 60, 1, 128, 124, 31, 255, 255, 248, 7, 255, 255, 224, 0, 0, 0, 0};
const uint8_t One_mask[] PROGMEM = { 3, 255, 255, 192, 31, 255, 255, 248, 63, 255, 255, 252, 124, 0, 0, 62, 120, 0, 0, 14, 240, 0, 240, 15, 240, 1, 240, 15, 224, 7, 240, 7, 224, 31, 240, 7, 224, 127, 240, 7, 224, 255, 240, 7, 224, 255, 240, 7, 224, 241, 240, 7, 224, 65, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 224, 1, 240, 7, 240, 1, 240, 7, 112, 1, 240, 15, 112, 1, 240, 14, 120, 0, 224, 30, 63, 255, 95, 252, 31, 255, 255, 248, 3, 255, 255, 192, 0, 0, 0, 0};
const uint8_t Two_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 248, 63, 255, 255, 252, 124, 0, 0, 62, 120, 7, 240, 14, 112, 31, 252, 14, 112, 127, 254, 6, 96, 255, 255, 7, 224, 254, 63, 7, 224, 240, 15, 135, 224, 224, 15, 135, 224, 192, 7, 135, 224, 0, 7, 135, 224, 0, 7, 135, 224, 0, 7, 135, 224, 0, 15, 135, 224, 0, 15, 135, 224, 0, 31, 7, 224, 0, 63, 7, 224, 0, 126, 7, 224, 0, 252, 7, 224, 3, 248, 7, 224, 7, 240, 7, 224, 15, 224, 7, 224, 63, 128, 7, 224, 63, 0, 7, 224, 126, 0, 7, 224, 252, 0, 7, 96, 248, 0, 7, 97, 240, 0, 7, 97, 240, 0, 7, 97, 255, 255, 199, 113, 255, 255, 199, 113, 255, 255, 206, 113, 255, 255, 206, 120, 0, 0, 30, 63, 0, 0, 252, 31, 255, 255, 248, 3, 255, 255, 192, 0, 0, 0, 0};
const uint8_t Three_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 248, 63, 255, 255, 252, 124, 0, 0, 62, 112, 15, 240, 14, 112, 127, 252, 14, 112, 127, 254, 6, 96, 127, 255, 7, 224, 120, 63, 7, 224, 96, 31, 7, 224, 0, 15, 7, 224, 0, 15, 135, 224, 0, 15, 135, 224, 0, 15, 7, 224, 0, 31, 7, 224, 0, 31, 7, 224, 0, 126, 7, 224, 31, 252, 7, 224, 63, 248, 7, 224, 63, 252, 7, 224, 31, 255, 7, 224, 0, 127, 7, 224, 0, 31, 135, 224, 0, 15, 135, 224, 0, 7, 199, 224, 0, 7, 199, 96, 0, 7, 199, 96, 0, 7, 199, 96, 0, 15, 135, 96, 192, 15, 135, 96, 240, 63, 135, 96, 255, 255, 7, 96, 255, 254, 6, 112, 255, 252, 14, 112, 63, 240, 14, 120, 3, 0, 30, 63, 0, 0, 252, 31, 255, 255, 248, 3, 255, 255, 128, 0, 0, 0, 0};
const uint8_t Four_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 248, 63, 255, 255, 252, 124, 0, 0, 62, 112, 0, 60, 14, 112, 0, 124, 14, 112, 0, 124, 6, 96, 0, 252, 7, 224, 0, 252, 7, 224, 1, 252, 7, 224, 3, 252, 7, 224, 3, 252, 7, 224, 7, 252, 7, 224, 15, 252, 7, 224, 15, 188, 7, 224, 31, 60, 7, 224, 62, 60, 7, 224, 62, 60, 7, 224, 124, 60, 7, 224, 248, 60, 7, 225, 248, 60, 7, 225, 240, 60, 7, 227, 224, 60, 7, 231, 255, 255, 199, 239, 255, 255, 231, 239, 255, 255, 231, 111, 255, 255, 231, 103, 255, 255, 199, 96, 0, 60, 7, 96, 0, 60, 7, 96, 0, 60, 7, 96, 0, 60, 7, 96, 0, 60, 6, 112, 0, 60, 14, 112, 0, 60, 14, 120, 0, 0, 30, 63, 0, 0, 252, 31, 255, 255, 248, 3, 255, 255, 128, 0, 0, 0, 0};
const uint8_t Five_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 240, 63, 255, 255, 252, 60, 0, 0, 62, 112, 31, 254, 14, 112, 127, 254, 14, 96, 127, 254, 6, 96, 127, 254, 6, 96, 127, 254, 7, 96, 120, 0, 7, 96, 120, 0, 7, 96, 120, 0, 7, 96, 120, 0, 7, 96, 120, 0, 7, 96, 248, 0, 7, 96, 248, 0, 7, 96, 255, 224, 7, 96, 255, 248, 7, 96, 255, 252, 7, 96, 255, 254, 7, 96, 0, 255, 7, 96, 0, 31, 7, 96, 0, 31, 7, 96, 0, 15, 135, 96, 0, 15, 135, 96, 0, 15, 135, 96, 0, 15, 135, 96, 0, 15, 135, 96, 0, 15, 135, 96, 0, 31, 7, 96, 224, 127, 7, 96, 255, 254, 7, 96, 255, 252, 6, 112, 255, 248, 6, 112, 255, 224, 14, 56, 15, 0, 30, 63, 0, 0, 252, 31, 255, 255, 248, 1, 255, 255, 0, 0, 0, 0, 0};
const uint8_t Six_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 240, 63, 255, 255, 252, 60, 0, 0, 62, 112, 3, 252, 14, 112, 15, 254, 14, 96, 31, 254, 6, 96, 63, 254, 6, 96, 127, 6, 6, 96, 124, 0, 7, 96, 248, 0, 7, 96, 240, 0, 7, 97, 240, 0, 7, 97, 240, 0, 7, 97, 224, 0, 7, 99, 227, 224, 7, 99, 239, 248, 7, 99, 255, 254, 7, 99, 255, 254, 7, 99, 252, 63, 7, 99, 248, 31, 135, 99, 240, 15, 135, 99, 224, 7, 135, 99, 224, 7, 135, 99, 224, 7, 135, 99, 224, 7, 199, 97, 224, 7, 135, 97, 224, 7, 135, 97, 240, 15, 135, 97, 248, 15, 135, 96, 252, 63, 7, 96, 127, 255, 6, 96, 127, 254, 6, 112, 63, 252, 6, 112, 15, 240, 14, 56, 0, 0, 30, 62, 0, 0, 124, 31, 255, 255, 248, 0, 127, 192, 0, 0, 0, 0, 0};
const uint8_t Seven_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 248, 63, 255, 255, 252, 124, 0, 0, 62, 120, 0, 0, 14, 113, 255, 255, 206, 115, 255, 255, 198, 99, 255, 255, 199, 225, 255, 255, 199, 224, 0, 15, 135, 224, 0, 15, 7, 224, 0, 31, 7, 224, 0, 30, 7, 224, 0, 62, 7, 224, 0, 60, 7, 224, 0, 124, 7, 224, 0, 120, 7, 224, 0, 248, 7, 224, 0, 240, 7, 224, 1, 240, 7, 224, 1, 224, 7, 224, 3, 224, 7, 224, 3, 224, 7, 224, 3, 192, 7, 224, 7, 192, 7, 224, 7, 192, 7, 224, 7, 128, 7, 224, 15, 128, 7, 96, 15, 128, 7, 96, 15, 0, 7, 96, 31, 0, 7, 96, 31, 0, 7, 112, 31, 0, 7, 112, 30, 0, 14, 112, 30, 0, 14, 120, 0, 0, 30, 63, 0, 1, 252, 31, 255, 255, 248, 3, 255, 255, 192, 0, 0, 0, 0};
const uint8_t Eight_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 248, 63, 255, 255, 252, 124, 0, 0, 62, 112, 7, 224, 14, 112, 31, 248, 14, 112, 127, 252, 6, 96, 127, 254, 7, 224, 252, 127, 7, 224, 248, 31, 7, 225, 240, 15, 7, 225, 240, 15, 135, 225, 240, 15, 135, 225, 240, 15, 135, 225, 240, 31, 7, 224, 248, 31, 7, 224, 254, 126, 7, 224, 127, 254, 7, 224, 63, 252, 7, 224, 127, 254, 7, 224, 255, 255, 7, 224, 252, 63, 7, 225, 240, 15, 135, 225, 224, 7, 135, 227, 224, 7, 199, 227, 224, 7, 199, 99, 224, 7, 199, 99, 224, 7, 199, 99, 224, 15, 199, 97, 240, 15, 135, 97, 248, 63, 135, 96, 255, 255, 7, 96, 255, 255, 6, 112, 127, 254, 14, 112, 31, 248, 14, 120, 3, 192, 30, 63, 0, 0, 252, 31, 255, 255, 248, 3, 255, 255, 128, 0, 0, 0, 0};
const uint8_t Nine_mask[] PROGMEM = { 0, 0, 0, 0, 15, 255, 255, 240, 63, 255, 255, 252, 60, 0, 0, 62, 112, 7, 240, 14, 112, 31, 252, 14, 96, 63, 254, 6, 96, 127, 254, 6, 96, 252, 63, 7, 96, 248, 31, 135, 97, 240, 15, 135, 97, 240, 7, 135, 97, 224, 7, 199, 97, 224, 7, 199, 97, 224, 7, 199, 97, 224, 7, 199, 97, 240, 7, 199, 97, 240, 7, 199, 97, 248, 15, 199, 96, 252, 63, 199, 96, 255, 255, 199, 96, 127, 255, 199, 96, 63, 255, 199, 96, 15, 247, 199, 96, 0, 7, 135, 96, 0, 7, 135, 96, 0, 15, 135, 96, 0, 15, 135, 96, 0, 31, 7, 96, 0, 63, 7, 96, 96, 126, 7, 96, 255, 252, 6, 96, 255, 252, 6, 112, 255, 248, 6, 112, 127, 224, 14, 56, 14, 0, 30, 62, 0, 0, 252, 31, 255, 255, 248, 0, 255, 255, 0, 0, 0, 0, 0};
const uint8_t Del_mask[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 255, 255, 255, 255, 255, 252, 0, 1, 255, 255, 255, 255, 255, 255, 255, 128, 3, 240, 0, 0, 0, 0, 0, 63, 192, 7, 128, 0, 0, 0, 0, 0, 249, 224, 14, 63, 248, 0, 0, 0, 0, 248, 112, 14, 63, 255, 128, 0, 0, 0, 248, 112, 28, 63, 255, 224, 0, 0, 0, 240, 48, 28, 63, 255, 248, 0, 0, 0, 248, 48, 28, 63, 255, 252, 0, 0, 0, 248, 56, 28, 60, 0, 252, 0, 0, 0, 248, 56, 24, 60, 0, 126, 0, 0, 0, 240, 56, 24, 60, 0, 63, 0, 0, 0, 240, 56, 24, 60, 0, 31, 0, 127, 128, 240, 56, 24, 60, 0, 15, 1, 255, 192, 240, 56, 24, 60, 0, 15, 131, 255, 224, 248, 56, 24, 60, 0, 15, 135, 255, 240, 240, 56, 24, 60, 0, 15, 143, 193, 240, 240, 56, 24, 60, 0, 15, 143, 128, 248, 240, 56, 28, 60, 0, 7, 143, 0, 248, 240, 56, 28, 60, 0, 7, 159, 0, 120, 248, 56, 28, 60, 0, 15, 159, 255, 248, 248, 56, 28, 60, 0, 15, 159, 255, 248, 248, 56, 28, 60, 0, 15, 159, 255, 248, 248, 56, 28, 60, 0, 15, 31, 255, 248, 240, 56, 28, 60, 0, 31, 30, 0, 0, 240, 56, 28, 60, 0, 31, 30, 0, 0, 240, 56, 28, 60, 0, 62, 31, 0, 0, 240, 56, 28, 62, 0, 124, 31, 0, 16, 240, 56, 28, 62, 1, 252, 15, 128, 112, 240, 56, 28, 63, 255, 248, 15, 225, 240, 240, 48, 12, 63, 255, 240, 7, 255, 240, 240, 48, 12, 63, 255, 192, 3, 255, 240, 240, 48, 14, 63, 255, 0, 1, 255, 224, 240, 112, 7, 63, 240, 0, 0, 127, 0, 112, 224, 7, 192, 0, 0, 0, 0, 0, 3, 224, 3, 252, 0, 0, 0, 0, 0, 255, 128, 0, 127, 255, 255, 255, 255, 255, 254, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t Min_mask[] PROGMEM = { 0, 0, 0, 0, 3, 255, 255, 192, 7, 255, 255, 224, 15, 0, 0, 240, 14, 0, 0, 112, 14, 0, 0, 112, 12, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 63, 252, 56, 28, 63, 254, 56, 28, 63, 254, 56, 28, 63, 254, 56, 28, 63, 252, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 12, 0, 0, 56, 12, 0, 0, 56, 14, 0, 0, 56, 14, 0, 0, 112, 15, 0, 0, 112, 7, 224, 15, 240, 7, 255, 255, 224, 0, 255, 255, 0, 0, 0, 0, 0};
const uint8_t Dot_mask[] PROGMEM = { 0, 0, 0, 0, 3, 255, 255, 192, 7, 255, 255, 224, 15, 0, 0, 240, 14, 0, 0, 112, 14, 0, 0, 48, 12, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 0, 0, 56, 28, 3, 0, 56, 28, 7, 192, 56, 28, 7, 192, 56, 12, 15, 192, 56, 12, 7, 192, 56, 12, 7, 128, 48, 14, 0, 0, 112, 15, 0, 0, 112, 7, 224, 15, 240, 7, 255, 255, 224, 0, 255, 255, 0, 0, 0, 0, 0};
const uint16_t Ok_arr_72x40[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2113, 4226, 8452, 10597, 14758, 16871, 18984, 19017, 21097, 21129, 21130, 23210, 23210, 23210, 23210, 23242, 23242, 23242, 23242, 23242, 23243, 23243, 23243, 25323, 23243, 23243, 25323, 25323, 25323, 25323, 25323, 25323, 25323, 25323, 25323, 25323, 25355, 25355, 25323, 25323, 25323, 23243, 23242, 21130, 21097, 16936, 14758, 10565, 4258, 2113, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4226, 14823, 25356, 33775, 38001, 40114, 40146, 40179, 42227, 42227, 42227, 42227, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42260, 42260, 42259, 42260, 42260, 42259, 42259, 42260, 42259, 42259, 42260, 42260, 42259, 42259, 42259, 42259, 42259, 42259, 42227, 42227, 42227, 40114, 38033, 33807, 27436, 14791, 4193, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 18984, 33808, 40147, 42259, 42260, 42260, 42260, 42260, 42259, 42259, 42259, 42259, 42227, 42227, 40179, 42227, 40179, 40179, 40179, 40179, 40179, 40179, 40179, 40179, 40178, 40179, 40179, 40179, 40179, 40179, 40179, 40178, 40178, 40178, 40178, 40178, 40179, 40178, 40178, 40178, 40178, 40146, 40146, 40178, 40211, 40179, 42227, 42227, 42259, 42259, 42260, 42260, 42260, 42292, 42259, 40146, 31695, 12677, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 23210, 40114, 42260, 44340, 42260, 42227, 38065, 33871, 29741, 27628, 23466, 21385, 19336, 17288, 17287, 15175, 15239, 15238, 15238, 15206, 13190, 13158, 13222, 13190, 13190, 13158, 13190, 13190, 15238, 15238, 15238, 15270, 15270, 17318, 38225, 46645, 46677, 46677, 48725, 48725, 46612, 23561, 15237, 15174, 15237, 15174, 15142, 17223, 19304, 21353, 23434, 27596, 33839, 38033, 42227, 42260, 44340, 42260, 38001, 12678, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14758, 40114, 42260, 42260, 40146, 31822, 19271, 11011, 8962, 6881, 6977, 6913, 7009, 9059, 25738, 31949, 38224, 40370, 40402, 38288, 34062, 23593, 11204, 928, 928, 864, 832, 2912, 2848, 4961, 7009, 7042, 9026, 9154, 11108, 50839, 57178, 57146, 57178, 57146, 57178, 57146, 34062, 6945, 6977, 6977, 4833, 4865, 4865, 4833, 4801, 2784, 2784, 2721, 8867, 19240, 33839, 42227, 42292, 42260, 33775, 4225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 29581, 42260, 44340, 40146, 21353, 8930, 6849, 6849, 6913, 6945, 9122, 23593, 42483, 52951, 55065, 57146, 57178, 57146, 57178, 57178, 57146, 55065, 52952, 44596, 25771, 9059, 864, 800, 2816, 2849, 4897, 7009, 6978, 9026, 13252, 50839, 59227, 59227, 59227, 57178, 59227, 57146, 34094, 6945, 6977, 6945, 6945, 4865, 4897, 4865, 2752, 2752, 2784, 640, 672, 672, 4770, 27596, 42227, 42260, 40147, 14823, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6339, 38001, 44340, 42227, 23466, 6849, 6945, 6881, 6945, 6945, 19464, 46644, 55033, 57178, 59227, 57179, 59227, 59227, 57179, 57179, 59227, 57178, 57178, 57179, 57178, 55033, 44596, 19496, 2881, 2944, 2880, 4993, 4961, 7041, 7074, 11171, 50871, 57178, 59227, 57179, 59227, 59227, 57146, 31982, 7009, 6945, 6977, 4929, 4897, 4897, 4833, 2817, 2720, 2784, 736, 736, 704, 672, 4738, 33872, 42260, 42260, 27436, 32, 0, 0, 0, 0, 0, 0, 0, 0, 12645, 40146, 42292, 38033, 10980, 6881, 6817, 6817, 8994, 29901, 52920, 57146, 57178, 57179, 57178, 57178, 59227, 57178, 59227, 57178, 57179, 57178, 59227, 59227, 59227, 59227, 57146, 52952, 29997, 2976, 2912, 4961, 4993, 6977, 7010, 11171, 50871, 57179, 59227, 59227, 57178, 57179, 57146, 31981, 7009, 6977, 7041, 4961, 4929, 4993, 2785, 4865, 2752, 2816, 672, 704, 800, 672, 608, 21385, 42259, 44340, 31695, 32, 0, 0, 0, 0, 0, 0, 0, 0, 16904, 42227, 42260, 31758, 6850, 6817, 6817, 6849, 36175, 55033, 57178, 57178, 57178, 57146, 59227, 59227, 59227, 57179, 57179, 57178, 57179, 57178, 57179, 57179, 59227, 57178, 59227, 57178, 55033, 32014, 5057, 2945, 4993, 7041, 7074, 11268, 50871, 57178, 57146, 59227, 57147, 59227, 57146, 32014, 6977, 7041, 4929, 4993, 4897, 4929, 4929, 2817, 2848, 2784, 736, 704, 672, 672, 640, 12997, 40146, 42260, 35888, 2145, 0, 0, 0, 0, 0, 0, 0, 0, 19049, 42227, 42259, 27564, 6849, 6817, 6881, 29901, 55033, 57178, 57178, 59227, 59227, 57146, 59227, 59227, 59227, 57178, 57179, 57146, 57178, 57179, 57178, 57179, 59227, 57178, 57179, 57179, 57178, 55033, 27916, 2881, 4993, 7041, 7138, 11235, 50871, 57179, 57179, 57179, 59227, 57178, 57146, 32014, 4993, 5057, 4929, 7073, 4897, 4929, 4865, 2785, 2816, 2848, 704, 800, 672, 672, 672, 6819, 38065, 44340, 38001, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 21130, 42259, 42259, 23402, 4769, 4769, 17254, 52920, 57178, 57179, 57178, 57179, 57146, 57178, 57178, 57179, 57146, 55065, 55033, 55033, 55065, 57146, 57178, 57178, 57179, 59227, 57178, 59227, 59227, 57178, 50871, 15398, 4993, 4993, 7074, 13348, 50871, 57178, 59227, 59227, 57178, 57178, 57146, 32013, 5025, 5025, 5025, 4961, 4993, 4865, 4929, 2881, 2848, 2752, 2784, 704, 736, 672, 672, 4834, 35953, 42260, 38001, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 23210, 42259, 42259, 21385, 6881, 6914, 44532, 57178, 57179, 57178, 57146, 57179, 57179, 57179, 57178, 52952, 42450, 25675, 17351, 15302, 23689, 38256, 50871, 57146, 57179, 57178, 57178, 57178, 57178, 59227, 57146, 42450, 5058, 5025, 7105, 11268, 50871, 57178, 57146, 57178, 57179, 57178, 57146, 32046, 5025, 5089, 4961, 5057, 4993, 5025, 2913, 2881, 2881, 2816, 2784, 672, 704, 704, 672, 4769, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 23210, 42259, 42259, 21321, 4897, 25674, 55065, 57178, 57178, 57178, 57146, 57179, 57178, 57178, 50839, 21513, 4898, 2913, 2849, 2913, 2977, 4961, 15302, 46644, 57146, 59227, 57179, 59227, 57178, 57178, 57179, 55032, 21640, 5025, 7105, 9219, 52919, 57146, 57146, 59227, 57178, 57179, 57146, 32046, 5057, 4961, 5025, 5057, 7138, 11204, 9124, 9091, 11140, 9028, 7011, 8963, 6914, 672, 672, 4769, 35920, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 23210, 42259, 42227, 21353, 6881, 44564, 57146, 57179, 57179, 57178, 57179, 57178, 59227, 52952, 21577, 7042, 4961, 4929, 5025, 3041, 3009, 3009, 3041, 9124, 48758, 57178, 57179, 57179, 59227, 57178, 57179, 57146, 38289, 4993, 7073, 11300, 52919, 57146, 57178, 59227, 57179, 59227, 57146, 29933, 5185, 4993, 4993, 11204, 44531, 52952, 52920, 52920, 52920, 52919, 52919, 52887, 46612, 11076, 704, 4801, 35952, 42292, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 23210, 42259, 42227, 21353, 15270, 52952, 57178, 57146, 57178, 57146, 57179, 57178, 57114, 34127, 7138, 7074, 4961, 5089, 5153, 5089, 3009, 3041, 2945, 2977, 25739, 55065, 57178, 57178, 59227, 57179, 57179, 57178, 50838, 7170, 7169, 11299, 50871, 57179, 57178, 57146, 57178, 57179, 57146, 32078, 5057, 5025, 4961, 38257, 57114, 57146, 57179, 57179, 57178, 57178, 57179, 57114, 40338, 2784, 2720, 4801, 33872, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 23210, 42259, 42259, 21353, 27756, 55065, 57146, 57178, 57179, 57178, 57146, 57178, 50871, 13253, 7170, 7105, 5025, 5025, 4993, 5025, 5025, 2977, 2945, 3073, 7171, 48758, 57178, 57179, 59227, 59227, 57178, 57178, 55033, 15398, 7137, 11236, 50871, 57178, 57178, 57179, 57178, 57146, 57146, 34126, 5089, 5089, 25675, 55001, 57178, 57178, 59227, 57179, 57179, 57146, 57146, 46612, 8963, 2720, 2784, 4801, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 21162, 42259, 42259, 21321, 38257, 57146, 57178, 57178, 57179, 57178, 57178, 57146, 42451, 7074, 7138, 5089, 5057, 4993, 5153, 5089, 5089, 5089, 2977, 2977, 5025, 38224, 57146, 57146, 57179, 57178, 59227, 57178, 57146, 27787, 5089, 11235, 52951, 57178, 57179, 57178, 57146, 57179, 57146, 32045, 5089, 13349, 50839, 57146, 57178, 59227, 57179, 57178, 57147, 57146, 50839, 15174, 2656, 2752, 2753, 4770, 33904, 42292, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 21130, 42259, 42259, 21353, 44532, 57146, 57179, 57146, 57179, 57179, 57146, 57146, 32046, 7010, 7137, 5121, 5057, 4961, 5057, 4993, 5089, 5025, 5025, 3105, 3009, 25739, 57146, 57179, 57178, 57178, 57179, 57178, 57146, 36207, 5153, 9219, 50871, 57179, 57178, 57179, 57179, 57178, 57146, 32045, 7170, 44628, 57178, 59227, 57179, 57179, 57146, 57179, 57146, 52920, 23626, 2784, 2753, 2817, 2785, 4802, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 21130, 42259, 42227, 23466, 48725, 57146, 57178, 57179, 57179, 57146, 57178, 55065, 23658, 7138, 7105, 5057, 5057, 5057, 5057, 5057, 4961, 4961, 5025, 2977, 2977, 19560, 55065, 57178, 57178, 57178, 57178, 57179, 57146, 38288, 5025, 9123, 50871, 57178, 59227, 57178, 57146, 57179, 57146, 29933, 36240, 57146, 59227, 57146, 57179, 57146, 57178, 57179, 55033, 29901, 2817, 2752, 2881, 2817, 2721, 4802, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 21129, 42259, 42259, 23466, 48758, 57178, 57179, 57178, 57178, 57146, 57146, 55065, 19560, 7138, 4993, 7169, 7105, 5025, 4993, 5089, 5025, 4993, 5057, 5025, 4993, 13285, 55065, 57178, 57179, 57178, 57178, 57179, 57178, 40402, 7138, 9187, 52951, 57179, 57179, 57178, 59227, 57179, 57114, 36240, 55065, 57179, 57146, 57147, 59227, 57179, 59227, 55065, 38256, 2817, 2785, 2753, 2785, 2817, 4833, 4802, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 21097, 42259, 42259, 23402, 48758, 57178, 57178, 57179, 57146, 57146, 57179, 55065, 19528, 7138, 7105, 5121, 5025, 5025, 5153, 4993, 5153, 5121, 5025, 4993, 5153, 17479, 55033, 57178, 57178, 57178, 57179, 57179, 57146, 40401, 5153, 9124, 52919, 57146, 57146, 57179, 57179, 57178, 57146, 52984, 57179, 57179, 57179, 57179, 57179, 57179, 57146, 42483, 4961, 2849, 2817, 2753, 2817, 2785, 4801, 4834, 35920, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 21097, 42227, 42259, 23402, 48758, 57178, 57179, 57178, 57178, 57146, 57179, 55065, 25738, 7010, 5057, 5089, 5025, 5089, 5057, 5025, 5057, 5153, 5025, 5121, 5089, 17511, 55033, 57179, 57179, 57178, 57146, 57178, 57146, 38288, 5090, 7107, 52951, 57146, 57178, 57146, 57178, 57178, 57178, 57178, 57179, 59227, 57179, 57146, 57146, 57146, 48725, 13189, 2849, 2849, 2849, 2817, 2753, 2721, 4833, 4802, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 19049, 42227, 42259, 23466, 46645, 57146, 57178, 57146, 57178, 57178, 57178, 57146, 31982, 7074, 5057, 5121, 7201, 5057, 5121, 5025, 5089, 5089, 5057, 5089, 5057, 23658, 55065, 59227, 57178, 57179, 57178, 57179, 57146, 32014, 5057, 9251, 52919, 57178, 57178, 57178, 57179, 57179, 57147, 57146, 57178, 57146, 57179, 57146, 57146, 50839, 15303, 2881, 2913, 2881, 4865, 4897, 2881, 2785, 2753, 4802, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 19017, 42227, 42259, 23434, 42419, 57146, 57178, 57178, 57146, 57146, 57146, 57178, 42451, 7074, 5089, 5121, 5057, 5057, 4993, 5121, 4993, 5057, 5057, 5089, 7010, 36240, 57146, 57178, 57178, 57178, 57179, 57178, 57146, 23722, 5089, 9155, 52919, 57178, 57146, 57178, 57178, 57147, 57146, 57146, 57178, 57179, 57146, 57179, 55065, 27852, 4961, 4961, 4929, 4865, 4865, 2881, 4865, 2753, 4865, 4770, 35952, 42260, 38033, 6338, 0, 0, 0, 0, 0, 0, 0, 0, 19049, 42227, 42259, 23434, 34030, 57146, 57178, 57178, 57179, 57146, 57179, 57179, 52920, 13381, 7138, 5025, 5057, 4961, 5121, 5057, 5089, 5057, 5089, 5089, 7202, 46677, 57146, 57146, 57179, 57146, 57146, 57178, 52984, 15398, 2977, 9155, 50871, 57146, 57178, 57146, 57178, 57178, 57146, 57179, 57146, 57146, 57178, 57146, 57146, 46644, 7042, 5057, 4929, 4929, 4897, 4929, 4833, 4801, 2753, 4834, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 19016, 42227, 42259, 23434, 21416, 55033, 57146, 57178, 57179, 57146, 57146, 57146, 57114, 38257, 7138, 4961, 5089, 5153, 5121, 5089, 5025, 5089, 5121, 4961, 23721, 55065, 57178, 57146, 57179, 57178, 57146, 57178, 48725, 5058, 5057, 11236, 50871, 57179, 57146, 57179, 57178, 57146, 57147, 57146, 57146, 57147, 57179, 57179, 57147, 57114, 40337, 7139, 4961, 4897, 4961, 2849, 4865, 2753, 2721, 4834, 35952, 42260, 38033, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 19016, 42227, 42259, 23402, 6818, 48693, 57146, 57178, 57146, 57146, 57146, 57146, 57179, 52952, 25803, 4993, 5025, 5025, 4993, 5057, 5121, 5057, 5089, 11300, 48725, 57146, 57179, 57146, 57179, 57178, 57178, 57146, 36207, 5057, 5057, 9156, 50871, 57178, 57178, 57178, 57146, 57146, 57146, 52984, 57146, 57146, 57146, 57146, 57179, 57146, 55033, 29901, 4897, 4961, 4833, 4865, 2785, 2785, 2753, 4802, 35952, 42260, 38001, 6338, 0, 0, 0, 0, 0, 0, 0, 0, 18984, 42227, 42259, 25483, 2656, 31918, 57114, 57178, 57146, 57178, 57146, 57146, 57146, 57178, 52920, 29932, 7106, 4993, 5025, 5057, 5089, 5089, 17479, 46677, 57146, 57146, 57146, 57178, 57179, 57146, 57179, 52920, 15398, 2913, 5121, 7139, 50839, 57146, 57179, 57179, 57146, 57178, 57146, 38289, 55065, 57147, 57147, 57179, 57146, 57146, 57146, 52920, 23690, 4929, 2881, 4865, 2849, 2785, 2721, 4770, 35952, 42292, 38001, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 16904, 42227, 42260, 25516, 2560, 6755, 48726, 57178, 57146, 57178, 57178, 57146, 57146, 57178, 57179, 55033, 46645, 34095, 27852, 25707, 31918, 42451, 52952, 57146, 57179, 57179, 57146, 57178, 57178, 57179, 57146, 38288, 2913, 2881, 3009, 9124, 50871, 57146, 57178, 57178, 57146, 57146, 57146, 29933, 42515, 57146, 57146, 57178, 57147, 57146, 57146, 57146, 48758, 15334, 4865, 4865, 2817, 2881, 2785, 6818, 35985, 42260, 38001, 4226, 0, 0, 0, 0, 0, 0, 0, 0, 14791, 42227, 42260, 27629, 2688, 4673, 25707, 55033, 57178, 57179, 57146, 57178, 57179, 57179, 57146, 57179, 57146, 57146, 57146, 57114, 57146, 57178, 57178, 57146, 57146, 57178, 57146, 57146, 57178, 57146, 48758, 9124, 3009, 2881, 2881, 9060, 50871, 57146, 57179, 57146, 57146, 57146, 57146, 29869, 9156, 48726, 57178, 57146, 57146, 57146, 57146, 57146, 57146, 44531, 11140, 2849, 2913, 2721, 2689, 6851, 38065, 44340, 35921, 4226, 0, 0, 0, 0, 0, 0, 0, 0, 12710, 40147, 42260, 31758, 2753, 2656, 4737, 38193, 55066, 57146, 57179, 57146, 57178, 57179, 57146, 57146, 57146, 57146, 57179, 57146, 57146, 57179, 57146, 57146, 57179, 57146, 57179, 57146, 57178, 52952, 21545, 2913, 2945, 2945, 2785, 8995, 52919, 57146, 57179, 57146, 57146, 57178, 57146, 29933, 2945, 21609, 52984, 57146, 57147, 57146, 57146, 57146, 57146, 55065, 38256, 4801, 2817, 2689, 2817, 10980, 40146, 42260, 35888, 2145, 0, 0, 0, 0, 0, 0, 0, 0, 8452, 40114, 42260, 35952, 4706, 2656, 2624, 6850, 42386, 55066, 57178, 57178, 57146, 57178, 57146, 57179, 57146, 57146, 57178, 57146, 57146, 57146, 57178, 57146, 57179, 57179, 57179, 57146, 52952, 27819, 2817, 2817, 768, 2913, 2977, 6947, 50839, 57146, 57147, 57146, 57146, 57146, 57146, 29901, 2913, 4962, 36143, 55065, 57178, 57178, 57146, 57147, 57146, 57146, 55033, 29868, 2784, 2752, 2688, 17160, 42227, 42260, 31695, 2113, 0, 0, 0, 0, 0, 0, 0, 0, 4226, 35921, 42260, 40147, 17127, 2624, 2592, 2624, 6819, 40338, 55033, 57146, 57178, 57146, 57178, 57146, 57178, 57178, 57146, 57146, 57178, 57178, 57179, 57178, 57178, 57146, 57146, 50871, 27852, 2817, 672, 736, 864, 832, 768, 7074, 50871, 57178, 57179, 57178, 57179, 57146, 57114, 27852, 2881, 2881, 9155, 44532, 57146, 57146, 57146, 57147, 57146, 57147, 57146, 52887, 17223, 672, 2657, 29645, 42260, 42260, 27436, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 29582, 42260, 42260, 33807, 6787, 544, 2624, 2592, 4706, 29773, 48758, 57114, 57146, 57146, 57178, 57178, 57178, 57179, 57146, 57178, 57178, 57179, 57178, 57146, 55033, 44531, 17287, 2912, 736, 672, 704, 736, 800, 768, 8995, 50839, 57146, 57178, 57178, 57146, 57146, 57114, 27820, 640, 864, 800, 13093, 50839, 57146, 57178, 57146, 57146, 57146, 57146, 57146, 48726, 13029, 17160, 40146, 42260, 42227, 16871, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16871, 40146, 42260, 42227, 31726, 10884, 608, 544, 544, 640, 11012, 33998, 48726, 55033, 57114, 57146, 57146, 57146, 57146, 57146, 57146, 55065, 52920, 42483, 21449, 4866, 2816, 672, 736, 736, 704, 800, 768, 672, 4770, 46645, 55033, 55033, 55065, 55065, 55033, 52952, 23530, 736, 736, 704, 704, 27788, 52920, 55033, 55033, 55033, 55033, 55033, 55033, 52920, 36080, 38065, 42260, 42260, 35888, 4258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2145, 29549, 42227, 42260, 42259, 38065, 29645, 19176, 10852, 6690, 2625, 2625, 4770, 13030, 29805, 38193, 42387, 44500, 44563, 40338, 34031, 23562, 11012, 2689, 2721, 672, 672, 2848, 800, 736, 672, 640, 704, 704, 2753, 13061, 21449, 19400, 19336, 21513, 21417, 19432, 2753, 640, 736, 608, 576, 2721, 19368, 21449, 21417, 23498, 25546, 27628, 31790, 38097, 42259, 42260, 42260, 40114, 16903, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6338, 29549, 40146, 42260, 42260, 42260, 42227, 40146, 38033, 35920, 33871, 31726, 29678, 29613, 29645, 27629, 27597, 27661, 27596, 27564, 27628, 27596, 27628, 27628, 27596, 27661, 27661, 27629, 27597, 27629, 27597, 27629, 29645, 27661, 29677, 29677, 29645, 29677, 29645, 29677, 29677, 29677, 29709, 29709, 29678, 31758, 31791, 33839, 33840, 35952, 38033, 40146, 42227, 42260, 42260, 42260, 42227, 35921, 16936, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2113, 16871, 29582, 38001, 40147, 42227, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42292, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42259, 42227, 40147, 38033, 33775, 23210, 6339, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 6339, 14758, 21097, 25323, 29549, 31662, 31695, 33775, 33808, 33808, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 35888, 33808, 33808, 35888, 33808, 33808, 33808, 33808, 33808, 33808, 33807, 33807, 33807, 33775, 33775, 31695, 31694, 31662, 29549, 27436, 23243, 19017, 12677, 6339, 2113, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2113, 32, 2113, 2113, 2113, 2145, 2145, 2145, 2145, 2145, 2145, 4193, 2113, 2145, 2113, 2113, 2113, 2113, 2113, 2113, 2145, 2113, 2113, 2113, 2113, 2113, 2145, 2113, 2113, 2113, 2113, 32, 2113, 2113, 32, 2113, 32, 32, 32, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t Ok_mask[] PROGMEM = { 0, 31, 255, 255, 255, 255, 255, 248, 0, 0, 255, 255, 255, 255, 255, 255, 255, 0, 3, 255, 255, 255, 255, 255, 255, 255, 128, 7, 255, 255, 255, 255, 255, 255, 255, 192, 7, 255, 255, 255, 255, 255, 255, 255, 224, 15, 255, 255, 255, 255, 255, 255, 255, 224, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 240, 15, 255, 255, 255, 255, 255, 255, 255, 224, 15, 255, 255, 255, 255, 255, 255, 255, 224, 7, 255, 255, 255, 255, 255, 255, 255, 224, 7, 255, 255, 255, 255, 255, 255, 255, 192, 3, 255, 255, 255, 255, 255, 255, 255, 192, 1, 255, 255, 255, 255, 255, 255, 255, 0, 0, 127, 255, 255, 255, 255, 255, 252, 0, 0, 0, 255, 255, 255, 255, 254, 64, 0};
const uint16_t OkP_arr_72x40[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 64, 160, 257, 353, 450, 514, 578, 611, 643, 643, 675, 707, 707, 707, 707, 707, 739, 739, 739, 739, 739, 739, 739, 771, 739, 739, 771, 771, 772, 772, 772, 772, 772, 772, 772, 772, 772, 772, 772, 772, 771, 739, 707, 675, 643, 546, 449, 321, 160, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 160, 482, 804, 1030, 3207, 3271, 3304, 3304, 3336, 3336, 3336, 3336, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3337, 3337, 3336, 3336, 3272, 3239, 1062, 836, 482, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 578, 1062, 3304, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3336, 3336, 3336, 3336, 3336, 3336, 3336, 3336, 3336, 3336, 3336, 3336, 3336, 3304, 3336, 3336, 3336, 3336, 5384, 5384, 5384, 5352, 5384, 5384, 3304, 5384, 5384, 5384, 5384, 3336, 3304, 3304, 3336, 3368, 3336, 3336, 3336, 3368, 3369, 3369, 3369, 3369, 3369, 3369, 3304, 997, 385, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 707, 3272, 3369, 3401, 3369, 5384, 5319, 7270, 7205, 7172, 7108, 5027, 5027, 4995, 4994, 4962, 4994, 4994, 2978, 2946, 2978, 866, 930, 897, 897, 2945, 2978, 2978, 4994, 7042, 7074, 7074, 7106, 9155, 11591, 17993, 18025, 18025, 17993, 18025, 15913, 9251, 7074, 6978, 7074, 4930, 4930, 4930, 4994, 4995, 2979, 3044, 3142, 3239, 3336, 3369, 3401, 3369, 3207, 417, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 450, 3271, 3369, 3369, 5352, 7269, 7011, 6914, 6913, 6913, 7009, 6913, 7009, 4961, 7331, 9445, 9574, 11719, 11751, 9606, 9477, 5155, 3041, 960, 928, 864, 864, 2912, 2880, 4993, 4961, 7073, 6977, 7137, 9090, 18091, 28495, 28495, 28495, 28495, 28495, 26414, 11557, 6945, 7009, 4929, 4833, 4865, 4865, 4865, 4801, 2784, 2784, 672, 673, 834, 1094, 3336, 3369, 3369, 1030, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 933, 3369, 3401, 5351, 7043, 6881, 6881, 6849, 6913, 6945, 7105, 7235, 13800, 18123, 22286, 24366, 26447, 26447, 26447, 26447, 26415, 24366, 20172, 13864, 5315, 2944, 896, 800, 2848, 2880, 4929, 7041, 6977, 6977, 7138, 18091, 28496, 28496, 28496, 28495, 28496, 26415, 11589, 6945, 7009, 4897, 6977, 4865, 4897, 2848, 2752, 2752, 2784, 640, 672, 672, 704, 996, 3336, 3369, 3304, 482, 0, 0, 0, 0, 0, 0, 0, 0, 0, 192, 3207, 3401, 3336, 5060, 6849, 6945, 6881, 6977, 6977, 7203, 13865, 22285, 26447, 26447, 26447, 28496, 26447, 28496, 28496, 28496, 28495, 28495, 28495, 26447, 22285, 13896, 3138, 864, 928, 2880, 5024, 4993, 7073, 7105, 7106, 18123, 28495, 28496, 28496, 28496, 28496, 26447, 9477, 7009, 4929, 4961, 4929, 4929, 4897, 4833, 2816, 704, 736, 736, 768, 704, 672, 672, 3174, 3369, 3369, 836, 32, 0, 0, 0, 0, 0, 0, 0, 0, 385, 3304, 3369, 5287, 6914, 6881, 6817, 6849, 6945, 9444, 18124, 24399, 26447, 26447, 26447, 26447, 26448, 26447, 28496, 26447, 28496, 28495, 28496, 28496, 28496, 28496, 26447, 20172, 7460, 960, 2944, 2944, 5025, 7009, 7041, 7106, 18091, 28496, 28496, 28496, 28495, 28496, 26415, 9445, 7041, 4929, 7073, 4961, 4961, 5025, 2816, 2816, 2752, 768, 704, 704, 800, 672, 640, 931, 3336, 3401, 997, 32, 0, 0, 0, 0, 0, 0, 0, 0, 546, 3336, 3369, 5157, 6849, 6849, 6817, 6881, 9541, 20237, 26447, 26447, 26447, 26447, 26448, 26448, 26448, 26447, 26447, 26447, 28495, 28496, 28495, 28495, 28496, 28495, 28496, 26447, 22253, 7428, 960, 2976, 5025, 5025, 7105, 7202, 18091, 28495, 28495, 28496, 28496, 28496, 26415, 9509, 7009, 7073, 4929, 5025, 2880, 4928, 2880, 2848, 800, 736, 736, 704, 672, 672, 640, 737, 3304, 3369, 3142, 96, 0, 0, 0, 0, 0, 0, 0, 0, 611, 3336, 3369, 5060, 6849, 6817, 6881, 7396, 20237, 26447, 26447, 26448, 26448, 26447, 26448, 26448, 26448, 26447, 26448, 26447, 26447, 26447, 26447, 28496, 28496, 28495, 28496, 28496, 28495, 22285, 5379, 2912, 2976, 4993, 7137, 7169, 18091, 28496, 28496, 28496, 28496, 28495, 26415, 9509, 4993, 5057, 4929, 5057, 4929, 2880, 2848, 2816, 768, 832, 704, 800, 704, 704, 672, 672, 3271, 3401, 3207, 160, 0, 0, 0, 0, 0, 0, 0, 0, 675, 3369, 3369, 4996, 4769, 4769, 7010, 18092, 26447, 26448, 26447, 26447, 26447, 26447, 26447, 26447, 24399, 20238, 18189, 18189, 20270, 24399, 26447, 26447, 26448, 28528, 26447, 28496, 28496, 26447, 18123, 3105, 2976, 4993, 7105, 7233, 18091, 28495, 28496, 28496, 28496, 28495, 26415, 9509, 5025, 5025, 5025, 2944, 5024, 2848, 2912, 832, 800, 736, 736, 736, 736, 672, 672, 768, 3207, 3369, 3207, 160, 0, 0, 0, 0, 0, 0, 0, 0, 707, 3369, 3369, 5027, 4865, 6913, 11752, 24399, 26447, 26447, 26447, 26447, 26447, 26447, 24399, 16108, 7655, 3171, 993, 993, 3234, 7558, 16075, 24399, 26447, 26447, 26447, 28495, 26447, 28496, 26447, 11751, 3040, 5057, 5089, 7169, 18123, 26447, 28495, 28495, 28496, 28496, 26415, 9541, 5057, 5089, 4961, 3008, 2976, 3008, 896, 832, 832, 768, 768, 672, 704, 704, 672, 704, 3206, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 32, 707, 3369, 3369, 4963, 4929, 7267, 20237, 26447, 26447, 26447, 26447, 26447, 26447, 24399, 11946, 3106, 832, 896, 832, 896, 960, 896, 993, 9800, 22351, 26448, 26448, 26448, 26447, 26447, 26447, 20204, 5250, 5057, 5089, 5153, 18123, 26447, 28495, 28496, 28496, 28496, 24399, 9541, 5089, 4961, 5057, 3040, 3073, 3009, 2977, 929, 897, 865, 896, 801, 768, 672, 672, 2720, 3174, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 32, 707, 3369, 3336, 4995, 4865, 11784, 24399, 26447, 26447, 26447, 26448, 26447, 24399, 14028, 3170, 5025, 4993, 2912, 3008, 3072, 992, 992, 1024, 961, 11914, 24399, 26447, 26448, 26448, 26447, 26447, 24399, 9638, 2944, 5057, 7233, 18123, 26447, 28495, 28496, 28496, 28496, 24398, 7428, 5217, 5025, 2976, 3041, 9704, 18124, 20172, 20172, 20172, 20140, 20172, 20140, 15881, 2881, 704, 704, 3174, 3369, 3239, 224, 0, 0, 0, 0, 0, 0, 0, 0, 707, 3369, 3336, 4963, 5058, 16076, 24399, 26447, 26447, 26447, 26448, 26447, 20270, 5445, 5089, 5057, 4993, 5121, 5185, 3072, 3040, 1024, 928, 960, 3235, 20237, 26447, 26447, 26448, 26448, 26447, 26447, 16042, 3104, 5153, 5185, 18123, 26448, 28495, 28496, 28495, 28496, 24366, 7492, 3040, 2976, 2944, 7558, 22286, 28495, 28496, 28496, 28495, 28496, 28496, 26414, 11655, 2784, 2752, 2753, 3174, 3369, 3239, 224, 0, 0, 0, 0, 0, 0, 0, 0, 707, 3369, 3336, 4995, 7300, 20238, 24399, 26447, 26447, 26447, 26447, 24399, 14027, 5089, 5121, 5057, 5057, 5057, 4993, 5025, 3008, 3008, 928, 1056, 1056, 11882, 26447, 26448, 26448, 26448, 26447, 26447, 20237, 5186, 5121, 5121, 18091, 26447, 26447, 28496, 26447, 28495, 22350, 7524, 3072, 3072, 3171, 18157, 26447, 28496, 28496, 28496, 28496, 28495, 26415, 15913, 2849, 2752, 2784, 2753, 3206, 3369, 3239, 224, 0, 0, 0, 0, 0, 0, 0, 0, 675, 3369, 3369, 4963, 9606, 22319, 24399, 24399, 26447, 26447, 26447, 22319, 7655, 5057, 5121, 5121, 5057, 5025, 5185, 5121, 3040, 3072, 3008, 960, 960, 5478, 22351, 26447, 26448, 26447, 26448, 26447, 24366, 5283, 5121, 5121, 18123, 26447, 26448, 26447, 26447, 26447, 22318, 5412, 3072, 3137, 11947, 24399, 28495, 28496, 28496, 28495, 28496, 26447, 18059, 2914, 2688, 2784, 2784, 2721, 3206, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 0, 675, 3369, 3368, 4995, 11784, 24399, 24399, 24399, 26447, 26447, 24399, 20270, 5444, 4993, 5121, 5121, 5057, 4993, 5089, 5025, 5121, 3008, 3008, 1088, 992, 3235, 20270, 26447, 26447, 26447, 26447, 26447, 24367, 9605, 5153, 5153, 16075, 26447, 26447, 26448, 26448, 26447, 22350, 5444, 1024, 7784, 22351, 26448, 28496, 28496, 28496, 28496, 26447, 20172, 5219, 736, 2784, 2849, 2785, 2753, 3206, 3369, 3239, 224, 0, 0, 0, 0, 0, 0, 0, 0, 675, 3369, 3336, 5059, 11850, 24399, 24399, 24399, 24399, 24399, 24399, 18190, 5251, 5121, 5089, 5089, 5057, 5057, 5089, 5089, 4993, 4993, 3008, 3008, 960, 1154, 18189, 26447, 26447, 26447, 26447, 26448, 24399, 9606, 5057, 3009, 16075, 26447, 26448, 26447, 26447, 26448, 22318, 3332, 3493, 16174, 26447, 26447, 26448, 26448, 28496, 26448, 22253, 7396, 2848, 2752, 2913, 2817, 2753, 2753, 3206, 3369, 3239, 224, 0, 0, 0, 0, 0, 0, 0, 0, 643, 3369, 3369, 5059, 13962, 24399, 24399, 24399, 24399, 24399, 24399, 18189, 5251, 5121, 5025, 5153, 5089, 5057, 4993, 5121, 5057, 4993, 5089, 5056, 2976, 1025, 18189, 26447, 26447, 26447, 26447, 26448, 24399, 9703, 3072, 1024, 16075, 26447, 26447, 26447, 26448, 26448, 20270, 3493, 12044, 24399, 26447, 26448, 26448, 26448, 26448, 24366, 9606, 2848, 2784, 2752, 2817, 2849, 2785, 2785, 3206, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 0, 643, 3369, 3369, 2915, 13962, 24399, 24399, 24399, 24399, 24399, 24399, 18189, 5218, 5121, 5089, 5153, 5025, 5025, 5185, 2976, 5185, 5153, 5056, 5024, 3136, 3169, 18189, 26447, 26447, 26447, 26447, 26448, 24399, 9702, 1088, 960, 16075, 26447, 26447, 26448, 26447, 26447, 20271, 7915, 20303, 26447, 26447, 26448, 26448, 26448, 24367, 11784, 2913, 2848, 2849, 2785, 2817, 2785, 2785, 2817, 3174, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 0, 643, 3336, 3369, 2947, 11914, 24399, 24399, 24399, 24399, 24399, 24399, 18222, 5251, 4993, 5089, 5121, 5057, 5121, 5089, 5057, 5057, 5185, 5057, 5121, 5089, 3202, 18189, 26447, 26448, 26447, 26447, 26447, 24367, 7590, 992, 992, 16075, 26447, 26447, 26447, 26447, 26447, 20303, 16207, 26447, 26448, 26448, 26447, 26447, 26415, 15977, 2977, 2881, 2881, 2881, 2849, 2785, 2753, 2785, 2785, 3206, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 32, 611, 3337, 3369, 2979, 11817, 22351, 24399, 24399, 24399, 24399, 24399, 20270, 5381, 5057, 5089, 5153, 5185, 5057, 3104, 5057, 5121, 5121, 5089, 5121, 3008, 5251, 20270, 26447, 26447, 26447, 26447, 26448, 24398, 5412, 992, 1088, 16075, 26447, 26447, 26447, 26448, 26448, 22351, 24399, 26447, 26447, 26448, 26447, 26415, 18091, 5090, 2913, 2913, 2913, 2849, 2881, 2913, 2817, 2753, 2785, 3206, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 0, 611, 3336, 3369, 2979, 7623, 22351, 24399, 24399, 24399, 24399, 24399, 22351, 9704, 5057, 3073, 3105, 5089, 5089, 2976, 5153, 2976, 5089, 5089, 5121, 4993, 7589, 22318, 26447, 26447, 26447, 26448, 26447, 22318, 3266, 1024, 993, 16075, 26447, 26447, 26447, 26447, 26448, 24399, 26447, 26447, 26448, 26447, 26447, 22286, 7396, 2945, 2945, 2881, 2849, 2849, 2881, 2817, 2753, 2817, 2721, 3174, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 0, 611, 3336, 3369, 2979, 5381, 20270, 24399, 24399, 24399, 24399, 24399, 24399, 14028, 5218, 5121, 5025, 3041, 4992, 5120, 3040, 5089, 5056, 5089, 5121, 5185, 11881, 24399, 26447, 26447, 24399, 26447, 26447, 18156, 3137, 960, 961, 16043, 26447, 26447, 26447, 26447, 26447, 24399, 26447, 26447, 26447, 26447, 26447, 24367, 13897, 2977, 3041, 2881, 2913, 2849, 2881, 2817, 2785, 2785, 2817, 3206, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 0, 610, 3336, 3369, 2979, 3010, 16109, 24399, 24399, 24399, 24399, 24399, 24399, 20270, 7558, 3073, 2945, 5121, 3136, 3105, 3072, 3008, 3072, 3105, 2945, 5283, 18189, 24399, 24399, 26448, 26447, 24399, 24399, 13929, 992, 992, 1025, 16075, 24399, 26447, 26447, 26447, 26447, 22351, 22351, 26447, 26448, 26448, 26448, 26447, 24366, 9638, 3041, 2945, 2849, 2945, 2881, 2817, 2753, 2721, 2785, 3206, 3369, 3239, 192, 0, 0, 0, 0, 0, 0, 0, 0, 578, 3336, 3369, 2947, 2721, 11850, 22351, 24399, 24399, 24399, 24399, 24399, 24399, 16109, 5315, 2944, 3009, 3008, 2944, 3040, 3104, 3008, 5121, 3137, 11881, 22351, 24399, 24399, 24399, 24399, 24399, 22318, 5477, 992, 992, 961, 13995, 24399, 26447, 26447, 26447, 26447, 22319, 14060, 24399, 26447, 26447, 26447, 26447, 26447, 20237, 7396, 2881, 2945, 2785, 2817, 2785, 2817, 2753, 2753, 3206, 3369, 3207, 192, 0, 0, 0, 0, 0, 0, 0, 0, 578, 3336, 3369, 2980, 2688, 5317, 18222, 24399, 24399, 24399, 24399, 24399, 24399, 22351, 14028, 5380, 3041, 2976, 3008, 3040, 3072, 3072, 3170, 11849, 22319, 24399, 24399, 24399, 24399, 24399, 24399, 16075, 1089, 896, 1056, 1024, 13995, 24399, 26447, 26448, 26447, 26447, 22318, 5542, 18221, 24399, 26448, 26447, 26447, 26447, 26447, 18124, 5283, 2913, 2881, 2849, 2849, 2785, 2753, 2721, 3207, 3369, 3207, 160, 0, 0, 0, 0, 0, 0, 0, 0, 546, 3336, 3369, 3012, 2592, 2689, 11882, 22351, 24399, 24399, 24399, 24399, 24399, 24399, 22351, 18189, 9801, 5445, 5347, 5251, 5317, 9704, 16076, 22351, 24399, 24400, 24399, 24399, 24399, 24399, 22318, 5542, 896, 864, 992, 960, 14027, 24399, 24399, 24399, 26447, 24399, 22350, 3332, 7719, 22351, 26447, 26447, 26448, 26447, 26447, 24367, 16010, 3042, 2848, 2817, 2817, 2913, 2817, 2721, 3239, 3369, 3207, 160, 0, 0, 0, 0, 0, 0, 0, 0, 482, 3336, 3369, 3076, 2688, 2656, 3203, 16109, 22351, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 22351, 20271, 18222, 20270, 20270, 22351, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 22351, 11914, 961, 992, 864, 864, 897, 14027, 24399, 24399, 24399, 24399, 24399, 22318, 5316, 993, 11882, 24399, 26447, 26447, 26447, 26447, 26447, 24367, 11752, 2977, 2848, 2912, 2720, 2688, 2753, 3271, 3401, 3175, 128, 0, 0, 0, 0, 0, 0, 0, 0, 417, 3304, 3369, 3109, 2784, 2688, 2688, 5446, 18222, 22351, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 16076, 3138, 896, 928, 928, 768, 832, 14027, 24399, 24399, 24399, 24399, 24399, 22318, 5380, 928, 3202, 16108, 24399, 26447, 26447, 24399, 24367, 26447, 22286, 9574, 2752, 2848, 2688, 2848, 2817, 3304, 3369, 3142, 96, 0, 0, 0, 0, 0, 0, 0, 0, 289, 3272, 3369, 3206, 2688, 2688, 2624, 2784, 7591, 18222, 22351, 22351, 22351, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 22351, 16108, 3267, 800, 800, 768, 864, 928, 800, 13995, 24399, 24399, 24399, 24399, 24399, 22318, 5348, 896, 896, 5413, 20238, 24399, 24399, 24399, 24399, 24399, 24399, 20205, 5316, 736, 704, 640, 770, 3336, 3369, 997, 64, 0, 0, 0, 0, 0, 0, 0, 0, 160, 3175, 3369, 3304, 770, 576, 576, 2624, 704, 5542, 16141, 22351, 22351, 22351, 22351, 22351, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 24399, 22319, 13995, 3299, 800, 704, 768, 896, 864, 800, 960, 13995, 24399, 24399, 24399, 24399, 24399, 22318, 5348, 864, 864, 960, 9736, 22319, 24399, 24399, 24399, 24399, 24399, 24399, 18092, 2914, 704, 640, 997, 3369, 3369, 836, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 933, 3369, 3369, 3110, 673, 544, 576, 576, 608, 3172, 11914, 18222, 22319, 22351, 22351, 22351, 22351, 24399, 24399, 24399, 24399, 24399, 22351, 22319, 18189, 9704, 930, 864, 736, 704, 736, 768, 800, 800, 832, 13995, 24399, 24399, 24399, 24399, 24399, 20270, 3267, 672, 896, 832, 833, 13963, 22351, 24399, 24399, 24399, 24399, 24399, 24367, 13930, 2817, 770, 3304, 3369, 3336, 514, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 514, 3304, 3369, 3336, 3077, 673, 640, 576, 576, 672, 801, 3301, 9834, 16109, 18222, 20303, 22351, 22351, 22351, 22351, 20271, 18190, 14028, 7688, 995, 768, 800, 672, 736, 736, 704, 832, 800, 704, 672, 9801, 18157, 18189, 18189, 20238, 20237, 16076, 3075, 768, 768, 704, 736, 3203, 16076, 18189, 18157, 20238, 20238, 20238, 20206, 18125, 5382, 3271, 3369, 3369, 3142, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 901, 3337, 3369, 3369, 3271, 996, 770, 641, 576, 576, 608, 672, 769, 3204, 5446, 5543, 7656, 7688, 7591, 5349, 3107, 801, 672, 704, 704, 672, 832, 832, 768, 704, 672, 704, 704, 736, 801, 3042, 962, 2946, 3106, 3010, 3074, 736, 640, 768, 608, 576, 704, 962, 3042, 3010, 3043, 3043, 3076, 3141, 3303, 3336, 3369, 3369, 3271, 514, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 192, 901, 3304, 3369, 3369, 3369, 3336, 3304, 3239, 3174, 1126, 1029, 1029, 965, 996, 1028, 996, 1060, 996, 964, 1028, 996, 1028, 1028, 996, 1060, 1060, 1028, 996, 1028, 996, 996, 996, 1060, 1029, 1028, 996, 1028, 996, 1028, 1029, 1029, 1061, 1061, 1029, 1061, 1093, 1094, 1094, 3206, 3239, 3304, 3336, 3369, 3369, 3369, 3336, 3175, 546, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64, 514, 933, 3207, 3304, 3336, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3369, 3336, 3304, 3239, 1030, 707, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 192, 450, 643, 772, 900, 965, 997, 1030, 1062, 1062, 3142, 1094, 1094, 3142, 3142, 3142, 1094, 3142, 3142, 3142, 3142, 1094, 3142, 1094, 3142, 3142, 3142, 3142, 1062, 1062, 3142, 1062, 1062, 3110, 1062, 1062, 1062, 1062, 1062, 1062, 1030, 1030, 997, 997, 965, 901, 836, 739, 610, 385, 224, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 32, 64, 32, 64, 64, 96, 96, 96, 96, 96, 96, 96, 128, 96, 96, 96, 64, 64, 96, 64, 64, 96, 64, 64, 64, 64, 96, 96, 64, 96, 64, 64, 32, 64, 64, 32, 64, 32, 32, 32, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint16_t Return_arr_88x40[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 2113, 4226, 6339, 8452, 10532, 10597, 12678, 14758, 14791, 14823, 16871, 16903, 16904, 16904, 16936, 18984, 18984, 18984, 18984, 19016, 19016, 19016, 19017, 19017, 19017, 19017, 19049, 21097, 19049, 21097, 21097, 21097, 21097, 21097, 21097, 21097, 21129, 21097, 21129, 21129, 21130, 21130, 21130, 21130, 21130, 21130, 21130, 21130, 21162, 21162, 21130, 21130, 21130, 21129, 21129, 21097, 19049, 19017, 16936, 16903, 14791, 12645, 10532, 8419, 4225, 2113, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2113, 6339, 12710, 23210, 27469, 31695, 35888, 38001, 38034, 40114, 40114, 40146, 40147, 40147, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42259, 42259, 42227, 42227, 42227, 42227, 42227, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42259, 42227, 42259, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 40146, 40114, 38034, 38001, 33808, 29581, 23210, 14758, 4226, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2113, 16871, 29581, 38001, 40147, 42227, 42260, 42260, 42260, 44340, 42260, 42260, 42260, 42260, 42259, 42259, 42259, 42259, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42227, 42195, 42227, 42227, 42227, 42227, 42195, 42195, 42195, 42195, 42195, 42195, 42195, 42195, 42195, 42195, 42227, 42227, 42227, 42227, 42227, 42227, 42259, 42259, 42259, 42260, 42260, 42260, 42292, 42260, 42259, 40147, 35920, 25323, 8419, 0, 0, 0, 0, 0, 0, 0, 6371, 29549, 40146, 42260, 42260, 42260, 42260, 42227, 40146, 38033, 37968, 35823, 33710, 31597, 31532, 29451, 29418, 27338, 27305, 27273, 27273, 27241, 27241, 25192, 25160, 25160, 25128, 25128, 25128, 25128, 25096, 25096, 25095, 25095, 25063, 25063, 25063, 25063, 25063, 25095, 25127, 25127, 25127, 25127, 25127, 25127, 25127, 25127, 25127, 25095, 25095, 25095, 25063, 25095, 25095, 23014, 25063, 25062, 25062, 22982, 22982, 23015, 25063, 25063, 25063, 25095, 25128, 27241, 27306, 29419, 31564, 33710, 35856, 38001, 40146, 42259, 42260, 42260, 42260, 42227, 35920, 14758, 32, 0, 0, 0, 0, 4226, 31662, 42227, 42260, 42260, 42259, 38001, 31629, 27273, 22982, 20803, 20770, 20705, 20737, 20738, 20737, 20737, 20737, 20737, 20737, 20737, 20705, 18657, 18657, 18625, 20673, 18561, 18561, 18561, 18561, 18561, 18529, 18528, 18496, 18464, 18464, 18464, 18464, 18496, 18528, 18560, 20641, 20705, 20737, 20737, 20737, 20737, 20737, 20737, 20737, 20769, 20705, 20705, 20705, 20737, 20673, 20673, 20673, 20673, 20641, 18561, 18528, 18528, 18496, 18464, 18464, 18464, 18464, 18464, 18464, 18464, 18496, 18496, 18561, 20707, 22918, 27306, 33743, 40114, 42260, 42260, 42260, 38001, 12678, 0, 0, 0, 0, 23243, 42227, 44340, 42260, 40114, 29419, 20804, 18657, 20673, 20705, 18625, 20705, 20705, 20705, 20705, 20705, 20737, 20705, 20705, 20705, 20705, 20705, 20705, 20673, 18625, 20641, 18561, 18561, 18561, 18561, 20609, 18561, 18528, 18496, 18464, 18464, 18464, 20512, 18464, 18496, 20576, 20609, 20673, 20705, 20737, 20737, 20738, 20737, 20737, 20738, 20737, 20705, 20705, 20705, 20705, 20673, 20641, 20673, 20673, 20641, 20609, 18528, 20576, 20544, 20576, 18464, 18464, 20512, 18464, 18464, 18464, 18496, 18496, 18528, 18529, 18529, 18561, 18593, 25095, 35888, 42259, 42292, 42260, 31662, 2113, 0, 0, 4226, 35920, 44340, 44340, 40114, 25160, 18625, 18625, 20673, 20673, 20673, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20673, 20673, 20641, 20609, 20608, 20576, 20609, 20609, 18561, 20576, 20544, 20512, 20512, 18432, 18464, 20512, 20512, 20544, 20608, 20641, 20705, 20705, 20737, 20737, 20737, 20737, 20737, 20673, 20705, 20705, 20673, 20705, 20673, 20673, 20641, 20641, 20641, 20641, 20576, 20576, 18496, 18496, 20512, 18464, 20512, 18464, 20512, 18464, 18464, 18496, 18528, 18529, 18561, 18593, 18593, 18593, 20836, 37968, 42260, 42260, 40114, 10565, 0, 0, 12678, 40146, 42260, 42259, 31564, 18593, 18593, 18625, 20673, 20673, 20673, 20705, 22753, 20705, 22753, 20705, 20705, 22785, 20705, 20705, 20705, 20705, 20705, 20705, 20673, 20673, 20641, 20609, 20576, 20577, 20577, 20609, 20609, 20577, 20544, 20512, 20512, 20480, 20512, 20512, 20512, 20544, 20576, 20641, 20673, 20705, 20705, 20737, 20737, 22753, 20705, 20705, 20705, 20673, 20673, 20673, 20673, 20673, 20673, 20641, 20641, 20609, 20576, 20576, 20544, 20544, 20512, 20512, 20576, 20512, 20512, 20544, 20544, 20544, 20577, 18528, 20609, 20641, 20641, 20673, 20641, 27241, 42227, 42260, 42227, 21097, 0, 0, 19017, 42227, 42292, 40146, 22949, 18658, 20641, 20673, 20738, 22914, 20673, 22786, 22786, 22753, 22753, 22818, 22753, 22753, 20705, 22753, 20705, 22753, 20705, 20673, 20673, 20673, 20641, 20609, 20608, 20576, 20576, 20577, 20609, 20577, 20576, 20512, 20512, 20480, 20480, 20480, 20512, 20512, 20544, 20609, 22721, 22753, 22753, 20705, 20705, 20705, 20705, 20705, 20705, 20705, 20673, 20705, 20673, 20641, 20673, 20641, 20609, 20576, 20576, 20544, 20544, 20544, 20544, 20512, 20512, 20512, 20544, 20512, 20544, 20576, 20577, 20609, 20609, 20673, 20673, 18593, 20641, 20771, 38001, 42260, 42259, 27436, 0, 0, 23243, 42259, 42260, 37969, 23174, 40273, 42418, 44467, 44563, 44563, 46580, 46612, 46611, 42418, 38063, 29320, 22786, 22754, 22786, 22754, 22753, 22753, 22753, 20673, 22721, 22721, 20641, 20609, 20608, 20576, 22624, 20576, 20577, 22625, 20576, 20544, 20512, 22592, 20480, 22657, 20480, 20512, 22592, 20576, 22689, 22721, 22753, 22753, 22753, 22753, 22753, 22753, 22753, 22721, 20705, 20673, 22721, 22689, 20641, 22753, 20609, 22657, 20544, 20544, 20544, 20512, 20512, 20512, 20544, 20544, 20544, 20609, 20673, 20577, 20577, 20641, 20609, 20641, 20641, 20641, 20673, 20674, 35823, 42260, 42260, 29549, 0, 0, 25356, 42260, 42260, 35855, 29514, 55065, 57146, 57146, 57146, 57178, 57146, 57146, 57178, 57146, 55097, 52984, 44435, 27238, 22786, 22754, 22721, 22753, 22721, 22721, 22721, 20673, 22689, 22689, 22657, 22624, 20576, 22624, 20576, 22625, 22625, 22657, 25028, 35949, 46579, 44499, 24835, 20512, 22624, 22624, 22689, 22721, 22721, 22753, 22753, 22753, 22753, 22849, 22753, 22721, 22753, 22721, 20641, 22689, 22689, 22689, 22625, 20577, 22592, 22592, 22592, 22592, 22592, 20512, 22592, 20544, 20544, 22625, 20577, 22785, 20609, 20705, 20673, 20641, 20673, 20673, 20673, 20674, 33677, 42260, 42260, 31662, 32, 0, 27436, 42260, 42260, 33742, 29707, 55065, 57178, 57178, 57178, 57178, 57178, 57178, 57146, 57146, 57178, 57178, 57146, 46580, 24900, 22754, 22754, 22753, 22753, 22721, 22721, 22785, 22689, 22657, 22657, 22624, 22624, 22624, 22624, 22624, 22625, 42289, 52952, 55097, 57146, 52919, 25028, 22624, 22560, 22688, 22689, 22721, 22753, 22785, 22721, 22753, 22753, 22753, 22753, 22753, 22721, 22721, 22689, 22689, 22689, 22657, 22657, 22625, 22592, 22592, 22592, 22592, 22752, 22560, 22592, 22625, 22593, 22689, 22657, 22657, 22657, 22689, 20641, 20641, 22722, 20641, 20737, 20673, 31564, 42260, 42260, 31694, 32, 32, 27436, 42260, 42260, 33710, 27594, 55033, 57178, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57178, 57178, 57178, 55065, 35852, 24898, 24802, 22753, 24769, 22721, 22721, 22689, 22689, 22689, 22689, 22657, 22689, 22657, 22624, 22592, 22882, 48758, 57178, 57146, 57146, 52919, 24900, 22593, 22625, 24673, 22849, 24769, 24801, 22721, 22721, 24801, 22721, 22721, 22721, 22721, 22721, 22689, 22689, 22657, 22657, 22721, 24705, 22625, 22625, 22625, 22625, 22625, 22657, 22592, 22593, 22754, 22625, 22657, 22657, 22689, 22722, 22689, 24900, 22884, 22914, 20673, 20641, 20673, 31532, 42260, 42260, 31695, 32, 0, 27468, 42260, 42260, 33710, 27369, 55065, 57146, 57178, 57178, 55032, 38192, 38223, 46612, 55065, 57178, 57178, 57178, 57146, 44531, 24834, 24866, 24801, 24801, 24769, 24769, 27140, 29479, 29383, 24965, 25027, 22721, 22657, 22625, 24803, 24932, 48758, 57146, 57178, 57146, 52952, 29320, 27238, 27077, 24802, 27206, 29448, 29384, 29352, 24995, 22721, 24769, 24769, 29480, 33771, 35948, 33868, 29385, 24769, 24770, 33707, 35982, 38030, 38127, 31594, 33772, 44466, 46612, 42385, 29513, 42321, 42353, 42386, 44466, 33739, 40240, 48725, 50871, 50871, 48693, 38127, 22851, 20673, 29452, 42259, 42260, 33775, 32, 0, 27436, 42260, 42260, 33710, 27496, 55065, 57146, 57146, 57178, 55032, 25319, 20610, 22755, 46676, 57146, 57178, 57178, 57178, 48725, 24802, 24802, 24769, 24769, 33771, 46611, 52919, 55064, 55033, 52984, 48693, 33804, 24769, 24738, 44531, 52951, 55033, 57146, 57146, 57146, 55065, 52984, 52952, 48693, 33803, 52952, 55065, 55065, 55032, 38062, 24737, 24769, 26979, 50806, 55097, 55065, 55065, 50806, 24803, 29223, 52984, 55097, 57146, 57146, 52951, 55065, 57178, 57178, 55065, 42386, 55097, 57178, 57146, 57146, 52984, 57145, 57178, 57178, 57178, 57178, 55097, 46580, 22851, 31500, 42259, 42260, 33775, 32, 32, 27436, 42260, 42260, 33710, 27432, 55033, 57178, 57146, 57146, 55032, 27335, 22722, 22689, 44498, 57146, 57178, 57146, 57178, 48725, 24834, 24801, 24834, 40240, 55032, 57146, 57178, 57178, 57178, 57146, 57178, 55065, 42385, 24867, 50839, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 52951, 33740, 55065, 57178, 57178, 57146, 40304, 22689, 24769, 26948, 52919, 57178, 57178, 57178, 52951, 24803, 29352, 55033, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 55065, 42418, 57146, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 55097, 36014, 31532, 42260, 42260, 31695, 32, 0, 27436, 42259, 42260, 33710, 25159, 55032, 57146, 57146, 57178, 55065, 29448, 24867, 29416, 50871, 57178, 57178, 57178, 57146, 42321, 24866, 24802, 40207, 55065, 57178, 57178, 57178, 57146, 57178, 57178, 57178, 57178, 55065, 38061, 50838, 57146, 57146, 57178, 57178, 57146, 57146, 57146, 57178, 52951, 33868, 55097, 57178, 57178, 57146, 40272, 22689, 22689, 24931, 50871, 57178, 57178, 57178, 52919, 22851, 27207, 55032, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 55065, 42450, 57146, 57178, 57146, 57178, 57178, 57145, 55065, 57146, 57178, 57178, 57178, 57178, 48725, 31532, 42260, 42260, 31695, 32, 0, 25356, 42259, 42260, 33742, 25223, 55032, 57146, 57146, 57178, 55065, 46612, 46612, 50871, 57146, 57178, 57178, 57146, 52952, 31561, 24801, 29319, 52951, 57178, 57178, 57178, 55097, 48725, 48757, 55065, 57178, 57178, 57178, 50838, 46612, 52984, 55033, 57146, 57146, 57178, 55065, 50839, 50871, 46612, 33771, 55097, 57178, 57146, 57146, 42385, 24737, 24705, 24963, 50839, 57146, 57178, 57146, 52919, 25028, 27207, 55032, 57178, 57178, 57178, 57178, 55065, 48725, 46677, 50871, 42386, 57178, 57178, 57178, 57178, 55097, 42386, 29610, 44499, 57146, 57178, 57178, 57178, 52951, 31596, 42260, 42260, 31695, 0, 0, 25355, 42260, 42260, 33742, 23174, 52952, 57146, 57146, 57178, 57146, 57146, 57178, 57178, 57146, 57146, 57146, 55032, 38062, 24769, 24770, 42320, 57146, 57178, 57178, 57146, 42418, 24964, 24899, 44499, 57146, 57178, 57178, 55065, 31626, 27206, 48725, 57178, 57146, 57178, 55064, 29352, 24963, 24802, 31594, 55065, 57146, 57146, 57146, 42353, 24705, 24705, 24931, 50839, 57178, 57178, 57146, 52919, 24996, 27270, 52984, 57178, 57178, 57178, 57146, 40304, 20802, 20706, 25030, 38127, 57146, 57178, 57178, 57146, 50838, 22980, 20609, 29577, 55065, 57178, 57178, 57178, 52984, 33709, 42260, 42260, 31695, 32, 32, 25355, 42259, 42260, 33742, 23206, 52984, 57146, 57178, 57146, 57146, 57146, 57146, 57146, 57178, 57178, 52951, 35949, 22690, 24769, 24898, 48725, 57178, 57178, 57146, 55097, 48758, 48790, 48758, 50871, 57146, 57178, 57146, 55097, 35981, 22625, 46612, 57146, 57178, 57178, 55032, 27271, 22657, 22657, 33738, 55065, 57178, 57178, 57146, 42418, 22657, 24673, 24899, 50839, 57146, 57178, 57178, 52952, 25028, 27206, 55032, 57178, 57146, 57178, 52984, 25158, 20577, 20544, 20577, 38063, 57146, 57178, 57178, 57178, 46644, 20641, 20577, 27335, 52984, 57178, 57178, 57178, 52984, 33709, 42260, 42260, 31695, 32, 0, 25323, 42259, 42260, 33743, 23077, 52984, 57146, 57178, 57146, 57146, 57146, 57178, 57146, 57178, 57178, 55065, 38127, 24770, 24769, 27011, 50871, 57146, 57146, 57146, 57178, 57146, 57146, 57146, 57146, 57178, 57146, 57178, 57146, 40207, 24769, 46612, 57146, 57178, 57146, 55033, 29351, 24769, 24705, 31625, 55065, 57178, 57178, 57146, 42386, 22625, 24673, 24803, 50838, 57178, 57178, 57178, 52952, 25061, 27174, 52984, 57178, 57178, 57178, 52951, 22787, 22657, 22625, 22689, 38062, 57145, 57178, 57146, 57146, 46644, 20673, 20705, 27174, 52952, 57178, 57146, 57178, 55032, 33773, 42260, 42260, 31695, 32, 0, 25323, 42259, 42260, 33743, 20997, 52952, 57146, 57146, 57146, 55065, 44531, 50871, 57146, 57146, 57178, 57178, 52984, 33835, 24801, 24996, 50903, 57178, 57146, 57178, 57146, 57146, 57146, 57146, 57146, 57146, 57146, 57178, 57146, 40175, 24770, 46612, 57146, 57178, 57146, 55065, 31528, 24737, 24705, 31561, 55065, 57178, 57146, 57178, 44498, 24706, 24833, 27043, 50838, 57178, 57178, 57178, 52984, 25093, 27142, 52952, 57178, 57178, 57178, 50871, 24932, 22625, 22593, 24737, 38030, 57146, 57178, 57146, 57146, 46676, 22657, 20577, 27238, 52984, 57178, 57178, 57178, 55064, 33806, 42260, 42260, 31694, 0, 0, 25323, 42259, 42260, 33775, 21028, 52952, 57178, 57146, 57146, 55065, 31595, 29674, 52951, 57178, 57178, 57178, 57178, 50806, 27140, 22754, 50838, 57146, 57146, 57178, 57145, 52919, 50839, 50871, 50839, 50806, 50838, 48758, 48693, 31658, 24769, 46580, 57178, 57178, 57146, 55065, 31528, 24705, 24641, 31625, 55065, 57178, 57146, 57178, 46612, 24705, 24769, 27109, 52919, 57178, 57178, 57146, 52952, 25029, 27077, 52952, 57146, 57178, 57178, 50903, 24995, 22721, 22625, 22657, 35918, 55065, 57178, 57178, 57178, 46676, 20609, 20577, 24998, 52952, 57146, 57178, 57178, 55032, 33774, 42259, 42260, 31694, 32, 0, 23243, 42260, 42260, 33775, 20997, 52919, 57146, 57146, 57178, 55065, 31627, 20609, 38127, 55097, 57178, 57178, 57178, 55097, 42353, 22721, 46644, 57178, 57178, 57178, 57146, 42417, 25092, 24899, 22787, 24899, 25093, 40240, 31658, 22689, 22721, 44531, 57146, 57146, 57178, 55065, 33868, 24803, 29287, 29510, 52984, 57178, 57178, 57178, 52951, 29480, 27205, 40240, 55097, 57178, 57178, 57146, 52984, 25093, 27109, 52952, 57146, 57178, 57146, 50871, 22884, 20544, 22721, 22657, 35885, 55065, 57178, 57146, 57178, 46644, 20545, 20609, 25093, 52952, 57178, 57178, 57178, 55033, 33774, 42259, 42260, 31694, 0, 0, 23243, 42259, 42260, 33775, 18724, 50871, 57146, 57146, 57146, 55065, 31564, 20642, 20931, 46645, 57146, 57146, 57146, 57146, 52984, 31595, 38095, 55097, 57178, 57178, 57178, 55065, 46644, 40272, 40208, 44531, 52951, 55097, 40176, 22625, 22689, 42385, 57178, 57146, 57178, 57146, 52952, 50838, 50871, 29351, 50871, 57178, 57146, 57178, 57146, 52984, 52952, 55065, 57178, 57178, 57146, 57146, 52952, 27206, 24964, 52952, 57146, 57146, 57178, 52919, 22819, 22592, 22625, 22625, 35853, 55065, 57178, 57178, 57146, 46645, 18529, 20577, 22917, 52920, 57178, 57178, 57178, 55033, 33806, 42260, 42260, 31694, 0, 0, 23242, 42259, 42260, 33775, 18788, 50871, 57146, 57146, 57146, 55065, 31692, 20641, 20673, 27369, 52952, 57178, 57178, 57146, 57146, 50838, 27367, 50838, 57146, 57178, 57178, 57178, 57146, 57146, 57146, 57146, 57146, 57146, 40273, 20608, 22656, 35949, 55065, 57178, 57178, 57146, 57178, 57178, 55032, 27303, 42354, 57146, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 57178, 52984, 25190, 24997, 52920, 57178, 57146, 57178, 52951, 22787, 20544, 20577, 22657, 33805, 55065, 57178, 57178, 57178, 48693, 18561, 18529, 23013, 52920, 57178, 57178, 57178, 55065, 33806, 42260, 42260, 31662, 0, 0, 23210, 42259, 42260, 35855, 18723, 50871, 57146, 57146, 57146, 55065, 33709, 18561, 18561, 18561, 40273, 55097, 57146, 57178, 57146, 55097, 42353, 31723, 52952, 57146, 57146, 57178, 57178, 57146, 57178, 57146, 57178, 55097, 40208, 22657, 20608, 22980, 48758, 57146, 57178, 57178, 57178, 57178, 55065, 27368, 25158, 50871, 57146, 57178, 57178, 57146, 57178, 57146, 55097, 57146, 57146, 57178, 52984, 23046, 25029, 50871, 57178, 57146, 57146, 52951, 20739, 18497, 20545, 20577, 33772, 55065, 57146, 57146, 57146, 46644, 18625, 18496, 22885, 50839, 57146, 57146, 57146, 52984, 33806, 42260, 42260, 31662, 32, 0, 21130, 42259, 42260, 35856, 16611, 50838, 57146, 57146, 57146, 55033, 31693, 18561, 18528, 16480, 20965, 48758, 55066, 57146, 57146, 55097, 50871, 22982, 29578, 48725, 55065, 57146, 57146, 57146, 57146, 55097, 52984, 44564, 25190, 20609, 20608, 18560, 27529, 48725, 55065, 55097, 55097, 55065, 48758, 23045, 18528, 25319, 46612, 52984, 55065, 55065, 52952, 42419, 40305, 50838, 50838, 50806, 44532, 20771, 20770, 42353, 48725, 48693, 46645, 40305, 20706, 18496, 18464, 18496, 23013, 40241, 44467, 42386, 42354, 29676, 16481, 16416, 18529, 31756, 38160, 38128, 36079, 33934, 31629, 42260, 42260, 29582, 32, 0, 21097, 42259, 42292, 35888, 16739, 36047, 42419, 42419, 42418, 40241, 20772, 18529, 18528, 18528, 16480, 25320, 38159, 38095, 36079, 36015, 31724, 20771, 18528, 18755, 29610, 38128, 42386, 42451, 40273, 33966, 25352, 18594, 18496, 20576, 20576, 18528, 18496, 18851, 27529, 33901, 33869, 27434, 20996, 18464, 18464, 18464, 16481, 23175, 27530, 27497, 20997, 18497, 16384, 18626, 16610, 16610, 18497, 18496, 18496, 20641, 18561, 16513, 16416, 18529, 16416, 18464, 18496, 18496, 18528, 18529, 18561, 16448, 16416, 16416, 16416, 16448, 16448, 18722, 16480, 16481, 14433, 16513, 31630, 42260, 42260, 29549, 0, 0, 18984, 42227, 42292, 38001, 14498, 12320, 12385, 12352, 14433, 14400, 16480, 18496, 18496, 16448, 16480, 16480, 14368, 16448, 14496, 16448, 16416, 16448, 16480, 16480, 16448, 16416, 18529, 16577, 16448, 16448, 16416, 18464, 18464, 18528, 18528, 18496, 18496, 18496, 16416, 16384, 16384, 16416, 16416, 18432, 18432, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16416, 16384, 18464, 18464, 18496, 18592, 18496, 16448, 16448, 16448, 18496, 18496, 18496, 18528, 18529, 18496, 16448, 16480, 16448, 16480, 18528, 18528, 18528, 16480, 16480, 14432, 16513, 33775, 42260, 42260, 27468, 0, 0, 14791, 42227, 44340, 40114, 18757, 12320, 12320, 12320, 14368, 14368, 16416, 16448, 16448, 16448, 16480, 18528, 16448, 16448, 16416, 16416, 16416, 18496, 18528, 18528, 18528, 18528, 18464, 18528, 18528, 18464, 18496, 18496, 18496, 18496, 20576, 20544, 18464, 18464, 18432, 18496, 16384, 16384, 18432, 18464, 18432, 18528, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18464, 18464, 18464, 18464, 18464, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 18496, 16448, 16448, 18626, 37969, 42260, 42259, 23243, 0, 0, 8484, 40114, 42260, 42227, 25226, 14368, 14368, 14528, 14400, 16416, 16448, 16448, 16448, 16448, 18496, 16480, 16448, 16448, 18496, 18496, 18496, 18496, 18528, 18528, 18528, 18560, 18496, 18528, 18496, 18496, 18496, 18528, 18528, 18528, 18496, 18464, 18464, 18432, 18432, 18432, 18432, 18432, 18432, 18464, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18464, 18464, 18464, 18496, 18464, 18464, 18464, 16416, 18496, 18496, 18496, 18496, 18496, 16448, 16448, 16448, 16448, 16448, 16448, 23015, 40147, 44340, 42227, 18984, 0, 0, 2145, 35888, 42260, 42260, 35888, 16579, 14368, 14368, 14368, 14400, 16416, 16416, 16416, 16448, 16448, 16448, 16448, 16448, 18496, 16448, 16448, 16448, 18496, 18496, 18496, 18528, 18528, 18528, 18528, 18528, 18496, 18496, 18528, 18496, 18464, 18464, 18464, 18496, 18464, 18432, 18432, 18432, 18432, 18432, 16384, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 18432, 16384, 16384, 16384, 18432, 18432, 16384, 16384, 18432, 16384, 16384, 16384, 18464, 16416, 18464, 16416, 16416, 16416, 16416, 16416, 16416, 16416, 16416, 16416, 16416, 16546, 33742, 42260, 42260, 38034, 10532, 0, 0, 32, 25323, 42259, 42260, 42259, 31629, 16611, 14368, 14400, 14368, 14368, 14368, 16416, 16416, 16416, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 16448, 18496, 18496, 16448, 16416, 16416, 16416, 16416, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 18432, 16384, 16384, 18432, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16416, 16416, 16416, 16416, 16416, 16416, 16416, 16416, 16416, 16579, 31565, 42227, 44340, 42260, 29582, 32, 0, 0, 0, 8419, 35888, 42260, 44340, 42227, 35856, 23080, 16578, 14433, 14368, 14368, 14368, 14464, 14368, 16416, 16416, 16512, 16448, 16448, 16448, 16448, 16416, 16448, 16416, 16416, 16448, 16448, 16416, 16448, 16416, 16416, 16416, 16416, 16384, 16384, 16384, 16384, 16416, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384, 14336, 16384, 16384, 16416, 16448, 16448, 18692, 25225, 35888, 42227, 42260, 42260, 38001, 12645, 0, 0, 0, 0, 0, 12710, 35921, 42260, 42292, 42260, 42227, 38001, 31662, 27338, 23048, 18789, 16644, 16546, 14465, 16481, 14433, 16481, 14400, 14400, 14400, 16449, 16448, 16416, 16448, 16448, 16448, 16416, 14368, 16416, 16448, 16448, 16416, 16416, 16416, 16416, 14336, 16416, 16416, 16416, 16416, 16416, 16448, 16416, 16416, 16384, 16384, 16384, 16416, 16416, 16384, 16416, 16449, 16416, 16384, 16416, 14336, 16416, 16416, 16416, 16384, 16416, 16449, 16449, 16481, 16481, 16514, 16514, 16546, 16579, 18692, 18789, 23047, 25226, 29484, 35823, 40082, 42227, 42260, 44340, 42259, 38001, 14791, 0, 0, 0, 0, 0, 0, 0, 10532, 29549, 40114, 42259, 42260, 42260, 42260, 42259, 42227, 40146, 40082, 38001, 37968, 35888, 35855, 33775, 33743, 33742, 33710, 33710, 31662, 31662, 31630, 31630, 31630, 31630, 31630, 33710, 31662, 33710, 31662, 33710, 33710, 33710, 33710, 33710, 33710, 33710, 33710, 33710, 33710, 33710, 33710, 33710, 33742, 33742, 33742, 33742, 33742, 33742, 33742, 33742, 33743, 33743, 33743, 33743, 33775, 33775, 33775, 33775, 35855, 35855, 35856, 35888, 35920, 37969, 38001, 38033, 40114, 40146, 42227, 42259, 42260, 42260, 42260, 42260, 42227, 38033, 29549, 10532, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2113, 10532, 21162, 31662, 35920, 40114, 40147, 42227, 42259, 42259, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42259, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42260, 42259, 42259, 42227, 42227, 40147, 40114, 38001, 33807, 27469, 18984, 8452, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 4226, 8452, 14758, 16904, 21130, 25323, 27436, 27469, 29581, 29582, 31662, 31694, 31694, 31695, 31695, 31695, 33775, 33775, 33775, 33775, 33775, 33775, 33775, 33775, 33775, 31695, 31695, 31695, 31695, 31695, 31695, 31695, 31694, 31695, 31695, 31694, 31694, 31694, 31694, 31694, 31694, 31694, 31694, 31662, 31662, 31662, 31662, 31662, 31662, 31662, 31662, 31662, 31662, 29582, 29581, 29581, 29549, 29549, 27501, 27468, 25388, 25355, 23243, 23210, 19017, 16871, 12678, 8484, 6339, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 32, 32, 32, 2113, 32, 32, 32, 32, 2113, 32, 2113, 0, 32, 32, 2113, 0, 2113, 32, 0, 0, 32, 0, 32, 32, 32, 2113, 0, 32, 32, 0, 32, 0, 32, 32, 0, 0, 0, 2113, 32, 32, 0, 0, 0, 32, 32, 0, 0, 32, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint16_t ReturnP_arr_88x40[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2048, 4096, 6144, 8192, 10272, 10272, 12320, 14368, 14368, 14368, 16416, 16416, 16416, 16448, 16448, 18496, 18497, 18497, 18496, 18497, 18496, 18497, 18497, 18497, 18497, 18497, 18497, 20545, 18497, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 20545, 18497, 18497, 16448, 16416, 14368, 12320, 10240, 8192, 4096, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2048, 6144, 12320, 22593, 26721, 30850, 34978, 37059, 37059, 39139, 39139, 39139, 39139, 39139, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41219, 41187, 41219, 41219, 41219, 41219, 41219, 41187, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41187, 41219, 41187, 41187, 39139, 39139, 37059, 37058, 32930, 28801, 22593, 14368, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2048, 16416, 28802, 37059, 39139, 41219, 41219, 41219, 41220, 43268, 41220, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41219, 41187, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41187, 41219, 41187, 41187, 41219, 41219, 41219, 41187, 41187, 41187, 41219, 41219, 41219, 41219, 41219, 41220, 41220, 41219, 41219, 39139, 35010, 24673, 8192, 0, 0, 0, 0, 0, 0, 0, 6144, 28801, 39139, 41219, 41220, 41219, 41219, 41219, 41219, 39171, 37123, 35074, 33026, 30978, 30978, 30978, 28930, 28930, 26882, 26882, 26882, 26882, 26850, 26850, 24769, 24737, 24737, 26785, 26785, 26785, 24705, 24673, 26721, 26721, 24641, 24673, 24673, 24673, 24705, 24737, 24801, 24834, 24834, 24866, 24866, 26914, 24834, 26882, 24834, 26849, 24801, 26849, 24801, 26849, 24769, 24801, 24801, 24769, 24737, 24737, 22689, 24673, 24673, 24672, 24640, 24640, 24641, 28737, 26721, 28769, 30849, 32930, 35010, 37091, 39139, 41219, 41219, 41220, 41219, 41219, 35010, 14368, 0, 0, 0, 0, 0, 4096, 30850, 41219, 41220, 41219, 41219, 37091, 32994, 28898, 22753, 22753, 22753, 20705, 24801, 20737, 22785, 22785, 22785, 22785, 22785, 20737, 20705, 20705, 20705, 20673, 28865, 22657, 22657, 22657, 20609, 20609, 20577, 20576, 20544, 20512, 18464, 20512, 20512, 20544, 22624, 20608, 20641, 22753, 22785, 22785, 22785, 20737, 22785, 22785, 20737, 22753, 22753, 20705, 20705, 20705, 20673, 22721, 20673, 20673, 22689, 20608, 20576, 20576, 20544, 18464, 20512, 20512, 20480, 20480, 22528, 18464, 22560, 18464, 20544, 20544, 24705, 26785, 32962, 39171, 41219, 41219, 41219, 37059, 12320, 0, 0, 0, 0, 22593, 41187, 43268, 41219, 39139, 28866, 22721, 20673, 22721, 20705, 20673, 22753, 20705, 22753, 20705, 22753, 22753, 20705, 22753, 22753, 22753, 22753, 22753, 20673, 20673, 24737, 20609, 20609, 20609, 20609, 20609, 20609, 20576, 18496, 20512, 18464, 20512, 20512, 20512, 20544, 20576, 20609, 20673, 24801, 24833, 22785, 22785, 22753, 24833, 22785, 22785, 20705, 22753, 20705, 24801, 22721, 20641, 22721, 22721, 20641, 22656, 20576, 20544, 20544, 24608, 20512, 20480, 24608, 18432, 22528, 20512, 18464, 20512, 20544, 18529, 22624, 18561, 20609, 24737, 35042, 41219, 41220, 41219, 30850, 2048, 0, 0, 4096, 34978, 43268, 43268, 39171, 24769, 18593, 22721, 22721, 22721, 24769, 24801, 24801, 24801, 20705, 24801, 24801, 22753, 22753, 22753, 22753, 24801, 20705, 22753, 20673, 20673, 22689, 24705, 22656, 24672, 22657, 22657, 20609, 22624, 20544, 22560, 20512, 20480, 22528, 20512, 20512, 22592, 22656, 22689, 24801, 22753, 24801, 24801, 22753, 22753, 22753, 22721, 22753, 20705, 22721, 20705, 20641, 22721, 20641, 24737, 24737, 22657, 20576, 22592, 20544, 20512, 22528, 20480, 20512, 20512, 22560, 20512, 18464, 18496, 20544, 22624, 20577, 18561, 20609, 20609, 22689, 37091, 41219, 41219, 39139, 10272, 0, 0, 12320, 39139, 41219, 41219, 30914, 18593, 20641, 20673, 22721, 20673, 22721, 22721, 26849, 24801, 24801, 24801, 24801, 26849, 22753, 22753, 20705, 24769, 22753, 22721, 20673, 22689, 22689, 22657, 24672, 22624, 20576, 20609, 24705, 22625, 22592, 24608, 22560, 22528, 20480, 20480, 22560, 22560, 22624, 20641, 22721, 22753, 22753, 22753, 24801, 24801, 22753, 20705, 24801, 22721, 24769, 20673, 22721, 20673, 24737, 20641, 24705, 22624, 24672, 26688, 24640, 20512, 20512, 22528, 24576, 22528, 20512, 22560, 20512, 20544, 22624, 20544, 22625, 20641, 20641, 20641, 20641, 26786, 41219, 41220, 41187, 20545, 0, 0, 18497, 41219, 41220, 39171, 22689, 22689, 24737, 20673, 26785, 24769, 22721, 26817, 22721, 24769, 26849, 26849, 24769, 24769, 24801, 24769, 22753, 24769, 22721, 24769, 22721, 22721, 24737, 22656, 22624, 24672, 20576, 22625, 20577, 22625, 22624, 26656, 24608, 22528, 26624, 22528, 24576, 26656, 22592, 24704, 24769, 24801, 24801, 22753, 22753, 22753, 24801, 22753, 22753, 22753, 22721, 24801, 24769, 22689, 22689, 24737, 22624, 22624, 22624, 22592, 22560, 24608, 24608, 22528, 22528, 22528, 24608, 22560, 22560, 22592, 22624, 22625, 22657, 22657, 20641, 20641, 22689, 20674, 37091, 41219, 41219, 26721, 0, 0, 22625, 41219, 41219, 37091, 26785, 45379, 47491, 47491, 49571, 49572, 49604, 51652, 49572, 47491, 43330, 32993, 26849, 24769, 24769, 24801, 22721, 26817, 24769, 24769, 26817, 24737, 22689, 24704, 22624, 24672, 24640, 24672, 24673, 26721, 22624, 22592, 22560, 26624, 24576, 24608, 26624, 24576, 24608, 22624, 24737, 24769, 24801, 22721, 24801, 22721, 26849, 22721, 26817, 24769, 22721, 24769, 24769, 24737, 24705, 24704, 22656, 26720, 22592, 22560, 22560, 22528, 22528, 26624, 24576, 22560, 22560, 24640, 22592, 20576, 22624, 24705, 22657, 20609, 20641, 20641, 20641, 20641, 35042, 41220, 41219, 28801, 0, 0, 24673, 41219, 41219, 35010, 32993, 58023, 58087, 58087, 60135, 60168, 58119, 60167, 60168, 58119, 58087, 57990, 47491, 30945, 26817, 26817, 24769, 28865, 22721, 26817, 26817, 24737, 26785, 24704, 26752, 28768, 26688, 22624, 22624, 24672, 24673, 24640, 28768, 39137, 49571, 49539, 26656, 24576, 24608, 26720, 26753, 26817, 26817, 24769, 26817, 26817, 24769, 24769, 24769, 22721, 24769, 24737, 22689, 22689, 24737, 26752, 22624, 24640, 24640, 26656, 24608, 24576, 26624, 22528, 22560, 24608, 24608, 24640, 22592, 24672, 22625, 24705, 24705, 20641, 22689, 20641, 20673, 20673, 32994, 41219, 41219, 30850, 0, 0, 26721, 41219, 41220, 35010, 35041, 58023, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 58087, 49572, 32961, 24769, 26817, 24769, 24769, 26817, 26785, 28833, 28833, 26752, 26752, 24672, 24672, 26688, 26688, 24672, 24672, 45379, 55942, 58087, 60200, 55909, 30784, 24576, 26656, 28736, 24737, 24769, 26817, 24769, 26817, 26817, 28897, 28865, 24769, 24769, 26785, 24769, 24737, 24737, 24704, 26720, 24672, 26688, 22560, 24608, 24608, 24576, 28672, 22528, 22560, 28704, 22560, 24640, 22625, 24673, 24673, 26753, 24705, 24705, 26785, 22689, 24737, 20641, 30914, 41219, 41219, 30850, 0, 0, 26721, 41219, 41219, 32962, 35009, 58023, 60168, 60168, 60168, 60135, 58055, 58055, 60135, 60168, 60168, 60168, 60168, 58023, 39170, 24769, 26817, 26817, 26817, 26785, 26817, 26785, 26785, 24705, 26752, 26752, 26720, 26688, 26688, 24608, 24608, 53733, 60200, 60200, 60200, 55910, 28704, 26624, 26656, 28736, 24737, 26817, 26817, 26817, 26817, 28897, 26817, 28865, 26785, 24737, 26785, 24737, 26785, 26752, 26752, 24672, 26720, 26656, 26656, 26656, 24608, 26656, 24608, 26624, 22560, 26688, 24640, 24640, 26720, 24673, 24705, 24705, 28833, 26785, 24737, 24737, 22689, 22689, 30914, 41219, 41219, 30882, 0, 0, 26721, 41219, 41219, 32930, 28865, 57990, 60168, 60168, 60168, 57926, 43234, 45282, 51555, 57991, 60168, 60168, 60168, 58087, 49540, 28833, 28833, 26785, 30913, 26817, 26785, 30913, 32961, 32961, 28801, 30849, 28768, 26688, 28704, 26656, 28736, 53732, 60200, 60200, 60200, 55942, 32832, 30816, 30784, 26720, 30881, 35041, 30945, 35041, 28865, 24769, 26817, 26785, 32993, 39170, 39170, 37090, 35009, 26720, 24672, 37057, 41186, 41218, 43266, 34977, 39041, 47459, 51620, 45379, 34945, 45347, 45411, 47491, 47491, 37090, 43299, 53733, 53829, 55909, 51716, 43331, 24737, 20673, 30914, 41219, 41220, 32930, 0, 0, 26721, 41219, 41219, 32930, 32929, 57990, 60168, 60168, 60168, 57926, 30752, 28672, 26624, 51588, 60135, 60168, 60168, 60168, 51652, 26785, 28833, 32929, 28865, 39137, 51620, 55877, 57990, 58022, 55910, 51652, 39138, 28768, 28736, 49507, 55877, 58022, 60200, 60200, 60200, 58087, 55942, 55942, 51684, 39073, 55910, 58054, 58055, 55974, 43330, 26785, 28833, 30881, 53765, 58087, 58087, 58087, 53797, 26720, 30849, 55910, 58119, 60167, 58119, 55877, 58022, 60200, 60232, 58087, 47395, 58087, 60200, 60200, 60199, 57958, 58087, 60232, 60232, 60232, 60232, 58119, 49604, 26817, 30914, 41219, 41219, 32930, 0, 0, 26721, 41219, 41219, 32930, 30881, 57990, 60136, 60168, 60168, 57926, 30849, 26720, 26688, 49475, 60135, 60168, 60168, 60135, 51620, 28865, 28833, 26785, 45379, 57990, 60167, 60168, 60168, 60168, 60168, 60168, 58023, 47427, 28768, 55845, 60200, 60200, 60200, 60200, 60200, 60200, 60200, 60200, 55909, 39073, 58087, 60200, 60200, 58119, 45411, 26785, 24737, 32929, 55877, 60200, 60200, 60232, 55909, 28736, 32865, 57990, 60232, 60232, 60232, 60168, 60200, 60232, 60232, 58087, 47395, 60135, 60232, 60232, 60200, 60200, 60232, 60232, 60232, 60232, 60232, 60232, 58087, 41282, 30914, 41219, 41219, 30882, 0, 0, 26721, 41219, 41219, 32930, 28833, 57958, 60136, 60136, 60168, 57958, 34977, 26785, 35009, 53765, 60168, 60168, 60168, 60103, 45347, 28833, 26785, 45346, 58023, 60168, 60168, 60168, 60167, 60167, 60168, 60200, 60168, 58023, 41154, 53765, 60168, 60168, 60200, 60200, 60200, 60200, 60200, 60200, 55877, 39073, 58055, 60200, 60200, 60167, 45411, 28833, 26753, 26720, 55845, 60200, 60200, 60200, 55909, 26656, 30784, 57958, 60232, 60232, 60200, 60200, 60200, 60200, 60200, 58087, 49443, 60135, 60232, 60232, 60232, 60232, 60167, 58022, 60135, 60200, 60232, 60232, 60232, 51716, 30946, 41219, 41220, 30882, 0, 0, 24673, 41219, 41219, 32962, 26753, 57958, 58087, 60136, 60136, 57991, 49540, 51588, 55845, 60135, 60168, 60168, 58087, 55846, 37025, 28833, 32961, 55878, 60168, 60168, 60168, 57990, 53668, 53668, 57990, 60168, 60168, 60168, 53733, 49507, 57926, 57990, 60168, 60200, 60200, 58055, 55845, 55845, 51587, 39073, 58055, 60200, 60200, 60167, 47491, 28801, 28800, 30784, 55813, 60200, 60200, 60200, 55909, 30752, 34848, 57990, 60200, 60200, 60232, 60200, 58055, 51652, 51620, 55845, 47363, 60135, 60232, 60232, 60232, 58087, 47427, 36961, 49443, 58087, 60232, 60232, 60232, 55910, 32962, 41219, 41219, 30882, 0, 0, 24673, 41219, 41219, 32930, 24705, 55910, 60136, 58088, 60168, 60136, 60135, 60135, 60136, 60136, 60168, 60135, 57926, 43202, 28800, 26785, 47395, 58055, 60168, 60168, 58055, 49443, 32800, 32768, 49443, 60135, 60168, 60168, 57990, 39009, 32832, 51620, 60168, 60200, 60200, 57990, 32864, 30752, 28672, 39041, 58055, 60200, 60200, 58119, 47459, 28768, 28736, 32800, 53765, 60200, 60200, 60200, 55909, 30752, 32800, 57958, 60200, 60200, 60200, 58087, 45346, 24608, 26624, 28704, 45250, 60135, 60200, 60232, 60200, 53765, 24608, 26624, 34881, 57958, 60200, 60232, 60232, 55974, 32994, 41219, 41219, 30882, 0, 0, 24673, 41219, 41219, 32930, 26753, 55878, 60135, 60136, 60136, 60136, 60136, 60136, 60168, 60168, 60135, 55813, 43169, 24640, 32896, 30849, 51620, 60168, 60168, 60168, 60071, 53700, 53700, 53700, 53733, 58087, 60168, 60168, 58055, 41154, 30720, 51588, 60168, 60200, 60168, 57990, 32864, 28736, 28704, 39041, 58023, 60200, 60200, 60167, 47459, 30784, 28704, 30752, 55781, 60200, 60200, 60200, 55910, 30752, 30752, 57958, 60200, 60200, 60200, 57990, 30784, 26688, 24576, 24576, 41154, 58055, 60232, 60232, 60200, 51620, 26624, 28672, 30784, 57894, 60200, 60232, 60232, 55974, 35042, 41219, 41219, 30850, 0, 0, 24673, 41219, 41220, 34978, 24704, 55878, 60136, 60136, 60135, 60135, 58055, 60136, 60136, 60136, 60136, 57958, 43234, 28768, 30848, 30849, 55813, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60135, 43266, 28768, 51588, 60167, 60168, 58120, 57990, 34977, 32929, 26720, 36993, 58022, 60200, 60200, 58119, 47459, 28704, 28672, 28672, 53733, 60200, 60200, 60200, 55910, 30752, 30784, 57958, 60200, 60200, 60200, 55877, 28768, 26656, 22528, 26624, 43202, 58055, 60200, 60200, 60200, 51620, 24576, 26624, 32800, 55846, 60200, 60200, 60200, 58022, 35042, 41219, 41219, 30850, 0, 0, 24673, 41219, 41219, 34978, 26720, 55846, 58087, 60136, 60135, 57959, 49475, 55813, 60103, 60136, 60136, 60136, 57926, 37057, 30849, 32897, 55813, 60168, 60168, 60168, 60168, 60168, 58120, 60168, 60168, 60168, 60168, 60168, 58055, 43298, 30848, 51555, 58119, 60168, 60168, 57990, 37057, 28800, 26688, 36961, 58022, 60200, 60200, 60168, 49507, 26624, 30720, 32768, 53733, 60168, 60200, 60200, 55910, 28736, 28704, 55878, 60200, 60200, 60200, 55877, 30817, 26656, 28672, 26624, 41122, 58055, 60200, 60200, 60200, 51620, 26624, 28672, 32800, 55846, 60200, 60200, 60200, 58022, 35042, 41219, 41219, 30850, 0, 0, 24673, 41219, 41219, 32930, 22624, 55846, 60136, 60136, 60136, 57959, 32865, 39009, 55813, 60135, 60136, 60136, 60135, 53701, 32897, 26753, 53733, 60136, 60168, 60168, 58055, 55845, 53765, 53765, 53765, 53733, 53733, 53732, 51652, 35009, 28801, 51587, 60135, 60168, 58120, 58022, 39041, 28736, 28672, 36928, 57990, 60200, 60168, 60168, 49539, 30720, 28672, 28704, 55813, 60200, 60200, 60200, 55910, 26688, 32800, 57926, 60200, 60200, 60200, 55877, 30784, 26624, 24576, 28672, 41122, 58055, 60200, 60200, 60200, 51620, 24576, 22528, 28704, 55814, 60200, 60200, 60200, 58022, 35042, 41219, 41220, 30850, 0, 0, 22625, 41219, 41219, 32930, 24672, 55845, 60135, 60136, 60136, 57959, 34913, 24576, 43202, 57991, 60136, 60136, 60136, 58023, 47427, 26752, 51588, 60135, 60136, 60168, 58055, 47363, 30720, 26656, 30720, 32768, 30752, 45314, 36993, 28768, 30849, 49507, 58087, 60168, 60168, 58023, 39073, 28704, 32832, 32832, 57958, 60168, 60168, 60168, 55877, 32864, 30752, 45282, 58023, 60200, 60200, 60200, 55910, 30784, 32832, 55846, 60200, 60200, 60200, 55877, 30752, 24576, 26624, 26624, 39041, 58023, 60200, 60200, 60200, 51620, 26624, 26624, 28704, 55814, 60200, 60200, 60200, 58022, 37090, 41219, 41220, 30850, 0, 0, 22625, 41219, 41219, 34978, 20544, 55813, 60135, 60136, 60136, 57959, 32897, 22592, 28736, 51588, 60103, 60136, 60136, 60136, 57926, 39105, 43234, 58023, 60136, 60168, 60168, 57990, 51555, 45282, 45282, 51523, 55813, 58023, 45314, 26720, 28801, 47427, 60135, 60168, 60168, 60135, 55878, 53733, 55813, 32832, 55813, 60168, 60168, 60168, 60167, 57958, 55845, 58023, 60168, 60168, 60200, 60168, 55910, 30752, 28704, 55846, 60168, 60200, 60200, 55877, 28736, 26624, 24576, 24576, 39041, 58023, 60200, 60200, 60168, 51652, 22528, 24576, 24608, 55814, 60168, 60200, 60200, 58022, 37090, 41219, 41219, 30850, 0, 0, 22593, 41219, 41219, 34978, 22592, 55813, 60136, 60136, 58088, 57959, 34945, 26688, 24672, 32865, 55846, 60136, 60136, 60136, 60103, 53701, 32897, 53733, 60135, 60168, 60136, 60168, 58087, 60103, 60103, 60135, 60168, 58087, 45346, 24672, 26720, 41186, 58023, 60168, 60168, 60168, 60168, 60168, 57990, 32864, 45347, 58055, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 60168, 55910, 30784, 28736, 55813, 60168, 60200, 60200, 55878, 26656, 24576, 22528, 24576, 37026, 58023, 60200, 60200, 60200, 51652, 22528, 24576, 28736, 55782, 60168, 60200, 60200, 58022, 37058, 41219, 41219, 30850, 0, 0, 22593, 41219, 41219, 35010, 22592, 53733, 60136, 58087, 58088, 57959, 37025, 22624, 22624, 22624, 45314, 58023, 60136, 60136, 60136, 58023, 47395, 37025, 55845, 58055, 60136, 60168, 60168, 60168, 60168, 60136, 60136, 58055, 45314, 28768, 24672, 28801, 53732, 60135, 60168, 60168, 60168, 60168, 57990, 30816, 30752, 53733, 60135, 60168, 60168, 60168, 60168, 60135, 57958, 60168, 60168, 60168, 55910, 28736, 28736, 53765, 60168, 60168, 60168, 55877, 22528, 24576, 22528, 22528, 36993, 57991, 60168, 58119, 58087, 51620, 20480, 22528, 28736, 53701, 58087, 58087, 58087, 55942, 35010, 41219, 41220, 30850, 0, 0, 20545, 41219, 41219, 35010, 18464, 53701, 58055, 60103, 58055, 57959, 34945, 20544, 22592, 22592, 24640, 53668, 58023, 60103, 58055, 58023, 55813, 28769, 32865, 51620, 57958, 58055, 58055, 60135, 58055, 58023, 55878, 49507, 28736, 26720, 22624, 22624, 32897, 51620, 57990, 58055, 58023, 57991, 53700, 30752, 24576, 32832, 49540, 55878, 57990, 57990, 55878, 47427, 45315, 53733, 53765, 53733, 49539, 26656, 26688, 47395, 51652, 51652, 51652, 45379, 26624, 24576, 22528, 22528, 26721, 45347, 47459, 47427, 45379, 34977, 22528, 22528, 20544, 35010, 43299, 41250, 41250, 37122, 30882, 41219, 41220, 28802, 0, 0, 20545, 41219, 41220, 35010, 20512, 39106, 47427, 47427, 47395, 43234, 22560, 22592, 20544, 20544, 18496, 28801, 43202, 41186, 43234, 41186, 37025, 22624, 22592, 22560, 36961, 43234, 47395, 49443, 47362, 41121, 30784, 22528, 20480, 26688, 26720, 24672, 22592, 22592, 34913, 39073, 39073, 34913, 26656, 20480, 24576, 20480, 18432, 28736, 32865, 32865, 26656, 22528, 20480, 20480, 20480, 20480, 22528, 22560, 22592, 22592, 22592, 20544, 20512, 22528, 22528, 20480, 18464, 20512, 22592, 20576, 22592, 18464, 20480, 18432, 16416, 18464, 18496, 20577, 24672, 16481, 16481, 20577, 32930, 41219, 41219, 28801, 0, 0, 18497, 41219, 41220, 37059, 16416, 14368, 16416, 22528, 16384, 22528, 20480, 20512, 20544, 20544, 20544, 22592, 20512, 22528, 20480, 16384, 22528, 24640, 20544, 20544, 22528, 22528, 20480, 22528, 20480, 20480, 20480, 20480, 22528, 22592, 22592, 22592, 20544, 20512, 18432, 22528, 18432, 20480, 22528, 22528, 20480, 22528, 20480, 20480, 18432, 18432, 22528, 20480, 22528, 18432, 20480, 18432, 22528, 18464, 22560, 24608, 24608, 20544, 20544, 20544, 18496, 22592, 22592, 20544, 20544, 20544, 20544, 20544, 20544, 20544, 18496, 20576, 18528, 20576, 18528, 18528, 14432, 20576, 34978, 41220, 41220, 26721, 0, 0, 14368, 41187, 43268, 39139, 20512, 14368, 14368, 18432, 16384, 18432, 18432, 22560, 18496, 20544, 18496, 22592, 16448, 18432, 20480, 20480, 20480, 22560, 20544, 22592, 20544, 24608, 22528, 24576, 24576, 22528, 20480, 20480, 24608, 24640, 24640, 26656, 22560, 22560, 18432, 22528, 22528, 20480, 22528, 24576, 20480, 22528, 20480, 20480, 20480, 22528, 20480, 22528, 22528, 20480, 18432, 22528, 20480, 22528, 22528, 22560, 20512, 20512, 22560, 20512, 22592, 18496, 22592, 24640, 22592, 20544, 22592, 20544, 20544, 18496, 22592, 20544, 18496, 18496, 18496, 18496, 18496, 18496, 37058, 41220, 41219, 22625, 0, 0, 8192, 39139, 41220, 41219, 24673, 18464, 14368, 16384, 16416, 18464, 20512, 18464, 20544, 22560, 22592, 18496, 20544, 18464, 22592, 22592, 20544, 20512, 20544, 18496, 20544, 22592, 20544, 20544, 22560, 20512, 22592, 22592, 24608, 24608, 20512, 20512, 20480, 22528, 20480, 20480, 20480, 20480, 22528, 22528, 18432, 22528, 20480, 24576, 24576, 20480, 18432, 20480, 20480, 18432, 24576, 24576, 20480, 20480, 20480, 18432, 18432, 20480, 20512, 20512, 18464, 20512, 22560, 20512, 18464, 18464, 18496, 18496, 18496, 18496, 20544, 18496, 18496, 18496, 18496, 16448, 18496, 24673, 39139, 43268, 41187, 18497, 0, 0, 2048, 34978, 41219, 41219, 34978, 18464, 14336, 18432, 16416, 16416, 18464, 20512, 20512, 20512, 16448, 22592, 18496, 18496, 24640, 18496, 18496, 18496, 20544, 18496, 18496, 22592, 22592, 22592, 20544, 20544, 18496, 20512, 20512, 18464, 18464, 20480, 22528, 20480, 20480, 22528, 18432, 20480, 20480, 20480, 20480, 22528, 20480, 20480, 20480, 22528, 20480, 18432, 20480, 20480, 20480, 20480, 18432, 20480, 18432, 20480, 20480, 20480, 18432, 20480, 16384, 18432, 18432, 20512, 22560, 20512, 16416, 18464, 18464, 18464, 18464, 18464, 18464, 18464, 18464, 16416, 20512, 32930, 41219, 41220, 37059, 10240, 0, 0, 0, 24673, 41219, 41219, 41219, 30850, 18464, 14336, 18464, 16416, 16416, 16416, 16416, 16416, 16416, 18464, 18464, 20544, 16416, 20544, 16448, 18464, 18496, 18496, 20544, 18496, 18464, 16448, 18464, 18464, 20512, 18464, 20512, 18432, 16384, 18432, 18432, 20480, 20480, 18432, 18432, 20480, 20480, 18432, 20480, 16384, 20480, 18432, 20480, 22528, 20480, 20480, 22528, 20480, 20480, 20480, 16384, 18432, 16384, 16384, 20480, 18432, 20480, 16384, 16384, 18432, 20480, 18432, 18432, 16384, 18464, 20512, 18464, 18464, 18464, 20512, 16416, 16416, 16416, 20512, 30849, 41219, 43268, 41219, 28802, 0, 0, 0, 0, 8192, 34978, 41219, 43268, 41219, 34978, 24641, 16416, 16384, 16384, 16384, 14368, 18464, 14368, 16416, 16416, 20512, 16416, 18464, 18464, 18464, 18464, 22560, 18464, 18464, 18464, 20512, 16416, 22560, 16416, 20512, 20480, 20480, 18432, 18432, 18432, 18432, 20480, 20480, 18432, 16384, 18432, 18432, 18432, 18432, 22528, 16384, 20480, 16384, 20480, 20480, 22528, 18432, 20480, 18432, 16384, 18432, 16384, 16384, 20480, 16384, 16384, 20480, 18432, 16384, 18432, 16384, 18432, 16384, 18432, 16384, 16384, 18432, 18432, 20480, 18432, 20512, 26689, 34978, 41219, 41220, 41219, 37059, 12320, 0, 0, 0, 0, 0, 12320, 35010, 41219, 41220, 41219, 41187, 37058, 32898, 26721, 24641, 20512, 16416, 16416, 16416, 18464, 16416, 20512, 16416, 16416, 16416, 18464, 16416, 18464, 18464, 18464, 20512, 18432, 18432, 18432, 24576, 20480, 18432, 18432, 16384, 18432, 16384, 22528, 16384, 16384, 16384, 16384, 20480, 18432, 18432, 18432, 18432, 20480, 16384, 16384, 20480, 16384, 16384, 18432, 16384, 16384, 16384, 18432, 16384, 16384, 18432, 16384, 16384, 16384, 18432, 18432, 16384, 16384, 18432, 18432, 20512, 20512, 24640, 24641, 28769, 34978, 39107, 41219, 41220, 43268, 41219, 37058, 14368, 0, 0, 0, 0, 0, 0, 0, 10240, 28801, 39107, 41219, 41219, 41219, 41219, 41219, 41187, 39139, 39107, 37059, 37058, 34978, 34978, 32930, 32930, 32930, 32930, 32898, 32898, 30850, 32898, 32898, 32898, 30850, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32898, 32930, 34978, 34978, 34978, 34978, 34978, 34978, 34978, 37058, 37058, 37058, 37059, 39107, 39139, 41187, 41219, 41219, 41219, 41219, 41219, 41187, 37059, 28801, 10240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2048, 10240, 20545, 30850, 34978, 39139, 39139, 41219, 41219, 41219, 41219, 41219, 41219, 41220, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41220, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41220, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41219, 41187, 39139, 39139, 37058, 32930, 26721, 18496, 8192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4096, 8192, 14368, 16448, 20545, 24673, 26721, 26753, 28801, 28802, 30850, 30850, 30850, 30882, 30850, 30850, 32898, 32930, 32930, 32930, 32930, 32898, 32898, 32930, 32930, 30850, 30882, 30882, 30882, 30882, 30882, 30882, 30850, 30850, 30882, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 30850, 28802, 28801, 28802, 28801, 28801, 26753, 26721, 24673, 24673, 22625, 22593, 18497, 16416, 12320, 8192, 6144, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2048, 0, 0, 0, 0, 2048, 0, 2048, 0, 0, 0, 2048, 0, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t Return_mask[] PROGMEM = { 0, 15, 255, 255, 255, 255, 255, 255, 255, 252, 0, 3, 255, 255, 255, 255, 255, 255, 255, 255, 255, 192, 15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 240, 31, 255, 255, 255, 255, 255, 255, 255, 255, 255, 248, 63, 255, 255, 255, 255, 255, 255, 255, 255, 255, 252, 63, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 63, 255, 255, 255, 255, 255, 255, 255, 255, 255, 252, 63, 255, 255, 255, 255, 255, 255, 255, 255, 255, 252, 31, 255, 255, 255, 255, 255, 255, 255, 255, 255, 248, 15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 240, 7, 255, 255, 255, 255, 255, 255, 255, 255, 255, 192, 0, 127, 255, 255, 255, 255, 255, 255, 255, 252, 0, 0, 0, 2, 20, 80, 8, 0, 128, 0, 0, 0};

class Chart_Gui : public Chart{
protected:
  Custom_Button Add_Data, Add_Point_B, Subtract_Point, Sample_Add, Calculate_B, Arrow_Right_B, Arrow_Left_B, Save_B;
  bool Add_Point_Menu_On;
  CH_TYPE (*Get_Current_Input_fun_ptr)(void);
  Custom_Button Edit_Points_X, Edit_Points_Y;
  Custom_Button One_Btn, Two_Btn, Three_Btn, Four_Btn, Five_Btn, Six_Btn, Seven_Btn, Eight_Btn, Nine_Btn, Zero_Btn, Dot_Btn, Del_Btn, Min_Btn, Ok_Button, Return_Button;
  String Coefficients_Option_str;
public:
  Chart_Gui(uint16_t X, uint16_t Y, int16_t width, int16_t height, uint8_t point_size, Adafruit_GFX* TFt) : Chart ( X, Y, width, height, point_size, TFt)/*, Edit_Points_X()*/{
        if (Tft == NULL);
        else Init_Buttons();
  }
  void Init_Buttons(){
     Save_B.initButton(1, 2, Save_mask, 0b1011110111110111, PRGUI_PINKRGB565, 48, 21, Tft);
     Add_Point_B.initButton(1, 2, Add_Point1_arr, Add_Point1_mask, Add_Point2_arr, Add_Point1_mask, 48, 50, Tft);
   //   Add_Point_B.initButton(1, 2, Add_Point2_arr, Add_Point1_mask, Add_Point2_arr, Add_Point1_mask, 48, 50, Tft);
     Subtract_Point.initButton(1, 2, Subtract_Point_arr, Subtract_Point_mask, Subtract_Point_arr2, Subtract_Point_mask, 48, 33, Tft);
#define CALCULAE_B_HEIGHT 27
     Arrow_Right_B.initButton(Top_Left_X+Width-11, Top_Left_Y+R_Menu_Bot-CALCULAE_B_HEIGHT-12, Arrow_Right_mask, 0b1011110111110111, PRGUI_PINKRGB565, 24, 15, Tft);
     Arrow_Left_B.initButton(Top_Left_X+Width-104, Top_Left_Y+R_Menu_Bot-CALCULAE_B_HEIGHT-12, Arrow_Left_mask, 0b1011110111110111, PRGUI_PINKRGB565, 24, 15, Tft);
     Calculate_B.initButton(Top_Left_X+Width-46, Top_Left_Y+R_Menu_Bot-11, Calculate_B_arr, Calculate_B_mask1, Calculate_B_arr2, Calculate_B_mask2, 56, 27, Tft);     /// <---- unnecesarry 2 masks, must ve been wrong bitmap export

     Get_Current_Input_fun_ptr = NULL;

     Edit_Points_X.initButton(Top_Left_X+16, Top_Left_Y+15, Arrow_Select_mask, 0b1011110111110111, 0b0000011111100000, 32, 20, Tft);
     Edit_Points_Y.initButton(Top_Left_X+16, Top_Left_Y+38, Arrow_Select_mask, 0b1011110111110111, 0b0000011111100000, 32, 20, Tft);
     Zero_Btn.initButton(Top_Left_X+52, Top_Left_Y + 210, Zero_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
  //   One_Btn.initButton(Top_Left_X, Top_Left_Y + 55, One_arr_32x40, One_mask, OneP_arr_32x40, One_mask, 32, 40, Tft);
     One_Btn.initButton(Top_Left_X+16, Top_Left_Y + 75, One_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Two_Btn.initButton(Top_Left_X+64, Top_Left_Y + 75, Two_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Three_Btn.initButton(Top_Left_X+112, Top_Left_Y + 75, Three_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Four_Btn.initButton(Top_Left_X+16, Top_Left_Y + 120, Four_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Five_Btn.initButton(Top_Left_X+64, Top_Left_Y + 120, Five_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Six_Btn.initButton(Top_Left_X+112, Top_Left_Y + 120, Six_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Seven_Btn.initButton(Top_Left_X+16, Top_Left_Y + 165, Seven_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Eight_Btn.initButton(Top_Left_X+64, Top_Left_Y + 165, Eight_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Nine_Btn.initButton(Top_Left_X+112, Top_Left_Y + 165, Nine_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Dot_Btn.initButton(Top_Left_X+160, Top_Left_Y + 165, Dot_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Del_Btn.initButton(Top_Left_X+174, Top_Left_Y + 75, Del_mask, 0b1011110111110111, 0b0000011111100000, 72, 40, Tft);
     Min_Btn.initButton(Top_Left_X+160, Top_Left_Y + 120, Min_mask, 0b1011110111110111, 0b0000011111100000, 32, 40, Tft);
     Ok_Button.initButton(Top_Left_X+112, Top_Left_Y + 220, Ok_arr_72x40, Ok_mask, OkP_arr_72x40, Ok_mask, 72, 40, Tft);
     Return_Button.initButton(Top_Left_X+194, Top_Left_Y + 220, Return_arr_88x40, Return_mask, ReturnP_arr_88x40, Return_mask, 88, 40, Tft);
     Add_Point_Menu_On = false;
  }
private:
  void Recalculate_Buttons_Positions(){
    Save_B.Change_Position(Top_Left_X+Width-24, Top_Left_Y+R_Menu_Top+10);
    Add_Point_B.Change_Position(Top_Left_X+Width-24, Top_Left_Y+R_Menu_Top+46);
    Subtract_Point.Change_Position(Top_Left_X+Width-24, Top_Left_Y+R_Menu_Top+87);
  /*  Arrow_Right_B.Change_Position(Top_Left_X+Width-23, Top_Left_Y+R_Menu_Bot-44);     ///<--- uncomment when add change chart height functionality
    Arrow_Left_B.Change_Position(Top_Left_X+Width-116, Top_Left_Y+R_Menu_Bot-44);
    Calculate_B.Change_Position(Top_Left_X+Width-74, Top_Left_Y+R_Menu_Bot-24);*/
  }
  String Add_X_Point_str;
  String Add_Y_Point_str;
  uint8_t X_Y_selection;
  void Draw_Add_Point_Menu(){
    if (Currently_Selected_Point < Regression_Model->Points_num){
      Add_X_Point_str = String(Regression_Model->X_points[Currently_Selected_Point]);
      Add_Y_Point_str = String(Regression_Model->Y_points[Currently_Selected_Point]);
    }
    else if (Get_Current_Input_fun_ptr == NULL){
      Add_X_Point_str = String("0");
      Add_Y_Point_str = String("0");
    }
    else{
      Add_X_Point_str = Get_Current_Input_fun_ptr();
      Add_Y_Point_str = String("0");
    }
    Tft->fillScreen(PRGUI_BLACK);
    Tft->setTextColor(PRGUI_CYAN);
    Tft->setTextSize(2);
    Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
    Tft->print(Add_X_Point_str);
    Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
    Tft->print(Add_Y_Point_str);
    X_Y_selection = 1;
    Edit_Points_X.drawButton(false);
    Edit_Points_Y.drawButton(true);
    Zero_Btn.drawButton(true);
    One_Btn.drawButton(true);
    Two_Btn.drawButton(true);
    Three_Btn.drawButton(true);
    Four_Btn.drawButton(true);
    Five_Btn.drawButton(true);
    Six_Btn.drawButton(true);
    Seven_Btn.drawButton(true);
    Eight_Btn.drawButton(true);
    Nine_Btn.drawButton(true);
    Dot_Btn.drawButton(true);
    Del_Btn.drawButton(true);
    Min_Btn.drawButton(true);
    Ok_Button.drawButton(true);
    Return_Button.drawButton(true);
  }
  void Refresh_Add_Point_Menu(int16_t cursor_x, int16_t cursor_y){
    if (Edit_Points_X.contains (cursor_x, cursor_y) && (Edit_Points_X.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
       if (X_Y_selection == 1){
        X_Y_selection = 0;
        Edit_Points_X.drawButton(true);
       }
       else {
        X_Y_selection = 1;
        Edit_Points_X.drawButton(false);
        Edit_Points_Y.drawButton(true);
       }
    }
    if (Edit_Points_Y.contains (cursor_x, cursor_y) && (Edit_Points_Y.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      if (X_Y_selection == 2){
        X_Y_selection = 0;
        Edit_Points_Y.drawButton(true);
       }
       else {
        X_Y_selection = 2;
        Edit_Points_Y.drawButton(false);
        Edit_Points_X.drawButton(true);
       }
    }
    if (One_Btn.contains (cursor_x, cursor_y) && (One_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      One_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '1';
        else Add_X_Point_str += "1";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '1';
        else Add_Y_Point_str += "1";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      One_Btn.drawButton(true);
    }
    if (Two_Btn.contains (cursor_x, cursor_y) && (Two_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Two_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '2';
        else Add_X_Point_str += "2";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '2';
        else Add_Y_Point_str += "2";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Two_Btn.drawButton(true);
    }
    if (Three_Btn.contains (cursor_x, cursor_y) && (Three_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Three_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '3';
        else Add_X_Point_str += "3";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '3';
        else Add_Y_Point_str += "3";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Three_Btn.drawButton(true);
    }
    if (Four_Btn.contains (cursor_x, cursor_y) && (Four_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Four_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '4';
        else Add_X_Point_str += "4";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '4';
        else Add_Y_Point_str += "4";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Four_Btn.drawButton(true);
    }
    if (Five_Btn.contains (cursor_x, cursor_y) && (Five_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Five_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '5';
        else Add_X_Point_str += "5";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '5';
        else Add_Y_Point_str += "5";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Five_Btn.drawButton(true);
    }
    if (Six_Btn.contains (cursor_x, cursor_y) && (Six_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Six_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '6';
        else Add_X_Point_str += "6";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '6';
        else Add_Y_Point_str += "6";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Six_Btn.drawButton(true);
    }
    if (Seven_Btn.contains (cursor_x, cursor_y) && (Seven_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Seven_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '7';
        else Add_X_Point_str += "7";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '7';
        else Add_Y_Point_str += "7";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Seven_Btn.drawButton(true);
    }
    if (Eight_Btn.contains (cursor_x, cursor_y) && (Eight_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Eight_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '8';
        else Add_X_Point_str += "8";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '8';
        else Add_Y_Point_str += "8";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Eight_Btn.drawButton(true);
    }
    if (Nine_Btn.contains (cursor_x, cursor_y) && (Nine_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Nine_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '9';
        else Add_X_Point_str += "9";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '9';
        else Add_Y_Point_str += "9";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Nine_Btn.drawButton(true);
    }
    if (Zero_Btn.contains (cursor_x, cursor_y) && (Zero_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Zero_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length() == 1 && Add_X_Point_str[0] == '0') Add_X_Point_str[0] = '0';
        else Add_X_Point_str += "0";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length() == 1 && Add_Y_Point_str[0] == '0') Add_Y_Point_str[0] = '0';
        else Add_Y_Point_str += "0";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Zero_Btn.drawButton(true);
    }
    if (Dot_Btn.contains (cursor_x, cursor_y) && (Dot_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Dot_Btn.drawButton(false);
      if (X_Y_selection == 1){
        uint8_t tmp = 0;
        for (int i=0; i<Add_X_Point_str.length(); i++){
          if (Add_X_Point_str[i] == '.') tmp++;
        }
        if (tmp<1){
          Tft->setTextColor(PRGUI_BLACK);
          Tft->setTextSize(2);
          Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
          Tft->print(Add_X_Point_str);
          Add_X_Point_str += ".";
          Tft->setTextColor(PRGUI_CYAN);
          Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
          Tft->print(Add_X_Point_str);
        }
      }
      else if (X_Y_selection == 2){
        uint8_t tmp = 0;
        for (int i=0; i<Add_Y_Point_str.length(); i++){
          if (Add_Y_Point_str[i] == '.') tmp++;
        }
        if (tmp<1){
          Tft->setTextColor(PRGUI_BLACK);
          Tft->setTextSize(2);
          Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
          Tft->print(Add_Y_Point_str);
          Add_Y_Point_str += ".";
          Tft->setTextColor(PRGUI_CYAN);
          Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
          Tft->print(Add_Y_Point_str);
        }
      }
      Dot_Btn.drawButton(true);
    }
    if (Del_Btn.contains (cursor_x, cursor_y) && (Del_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Del_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str.length()>1){
          Add_X_Point_str = Add_X_Point_str.substring(0, Add_X_Point_str.length()- 1);
        }
        else Add_X_Point_str = "0";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str.length()>1){
          Add_Y_Point_str = Add_Y_Point_str.substring(0, Add_Y_Point_str.length()- 1);
        }
        else Add_Y_Point_str = "0";
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Del_Btn.drawButton(true);
    }
    if (Min_Btn.contains (cursor_x, cursor_y) && (Min_Btn.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Min_Btn.drawButton(false);
      if (X_Y_selection == 1){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
        if (Add_X_Point_str[0] == '-') Add_X_Point_str = Add_X_Point_str.substring(1, Add_X_Point_str.length());
        else Add_X_Point_str = "-" + Add_X_Point_str.substring(0, Add_X_Point_str.length());
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+5);
        Tft->print(Add_X_Point_str);
      }
      else if (X_Y_selection == 2){
        Tft->setTextColor(PRGUI_BLACK);
        Tft->setTextSize(2);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
        if (Add_Y_Point_str[0] == '-') Add_Y_Point_str = Add_Y_Point_str.substring(1, Add_Y_Point_str.length());
        else Add_Y_Point_str = "-" + Add_Y_Point_str.substring(0, Add_Y_Point_str.length());
        Tft->setTextColor(PRGUI_CYAN);
        Tft->setCursor(Top_Left_X+55, Top_Left_Y+28);
        Tft->print(Add_Y_Point_str);
      }
      Min_Btn.drawButton(true);
    }
    if (Ok_Button.contains (cursor_x, cursor_y) && (Ok_Button.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Ok_Button.drawButton(false);
      Add_Point_Menu_On = false;
      CH_TYPE point_x = (CH_TYPE)Add_X_Point_str.toFloat();       ///<--------------------here is stupid string conversion forcing floats
      CH_TYPE point_y = (CH_TYPE)Add_Y_Point_str.toFloat();
      Regression_Model->Add_Point(point_x, point_y);
      for(uint16_t i=0; i<Regression_Model->Points_num; i++)
        if (Regression_Model->X_points[i] > point_x)
          for(uint16_t i=0; i<Regression_Model->Points_num; i++)
             if (Regression_Model->X_points[i] < point_x)
               for(uint16_t i=0; i<Regression_Model->Points_num; i++)
                 if (Regression_Model->Y_points[i] > point_y)
                   for(uint16_t i=0; i<Regression_Model->Points_num; i++)
                     if (Regression_Model->Y_points[i] < point_y){
                       Tft->fillScreen(PRGUI_BLACK);
                       Draw_Polynomial_Equation();
                       Draw_Axis();
                       Draw_Polynomial();
                       Draw_Points();
                       Draw_Buttons();
                       return;
                     }
      Recalculate_Chart_Scale_Wash_Screen();
      Draw_Polynomial_Equation();
      Draw_Axis();
      Draw_Polynomial();
      Draw_Points();
      Draw_Buttons();
      return;
    }
    if (Return_Button.contains (cursor_x, cursor_y) && (Return_Button.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Return_Button.drawButton(false);
      Add_Point_Menu_On = false;
      Tft->fillScreen(PRGUI_BLACK);
      Draw_Polynomial_Equation();
      Draw_Axis();
      Draw_Polynomial();
      Draw_Points();
      Draw_Buttons();
      return;
    }
  }
public:
  void Draw_Buttons(){
    if (Tft == NULL) return;
    Recalculate_Buttons_Positions();
    Calculate_B.drawButton(true);
    if (Regression_Model->Coefficients_Auto_Calculate_Num_Settings > 0xde38){
       uint16_t tmp = Regression_Model->Coefficients_Auto_Calculate_Num_Settings - 0xde38;
       Coefficients_Option_str = "lsqr" + String(8*tmp);
    }
    else Coefficients_Option_str = String(Regression_Model->Coefficients_Auto_Calculate_Num_Settings);
    Tft->setTextColor(PRGUI_CYAN);
    Tft->setTextSize(2);
    Tft->setCursor(Top_Left_X+Width-92, Top_Left_Y+R_Menu_Bot-CALCULAE_B_HEIGHT-19);
    Tft->print(Coefficients_Option_str);
    Arrow_Right_B.drawButton(true);
    Arrow_Left_B.drawButton(true);
    Save_B.drawButton(true);
    Add_Point_B.drawButton(true);
    Subtract_Point.drawButton(true);
/*
    Tft->drawFastVLine(Map_X_Point_to_Pixel(3.0f), Top_Left_Y+16, 300, PRGUI_YELLOW);
    Tft->drawFastHLine(Top_Left_X+16, Map_Y_Point_to_Pixel(3.0f), 390, PRGUI_YELLOW);
    Tft->drawFastVLine(Map_X_Point_to_Pixel(2.0f), Top_Left_Y+16, 300, PRGUI_YELLOW);
    Tft->drawFastHLine(Top_Left_X+16, Map_Y_Point_to_Pixel(2.0f), 330, PRGUI_YELLOW);
    Tft->drawFastVLine(Map_X_Point_to_Pixel(1.0f), Top_Left_Y+16, 300, PRGUI_YELLOW);
    Tft->drawFastHLine(Top_Left_X+16, Map_Y_Point_to_Pixel(1.0f), 330, PRGUI_YELLOW);
    Serial.println(Map_Y_Point_to_Pixel(3.0f));
    Serial.println(Map_Y_Point_to_Pixel(2.0f));
    Serial.println(Map_Y_Point_to_Pixel(1.0f));*/
  }
  void Set_Get_Current_Input_Fun(CH_TYPE (*fun_ptr)(void)){
    Get_Current_Input_fun_ptr = fun_ptr;
  }
  void Refresh_On_Cursor_Press(int16_t cursor_x, int16_t cursor_y){
    if (Tft == NULL) return;
    if (Add_Point_Menu_On == true){
      Refresh_Add_Point_Menu(cursor_x, cursor_y);
    }
    /*else if (Add_Data.contains(cursor_x, cursor_y)){
      Add_Data.drawButton(false);
    }
    else if (Sample_Add.contains(cursor_x, cursor_y)){
      Sample_Add.drawButton(false);
    }*/
    else if (Save_B.contains(cursor_x, cursor_y)){
      Save_B.drawButton(false);
      Regression_Model->Save_To_Eeprom();
      Save_B.drawButton(true);
    }
    else if (Add_Point_B.contains(cursor_x, cursor_y)){
      Add_Point_B.drawButton(false);
      Add_Point_Menu_On = true;
      Draw_Add_Point_Menu();
    }
    else if (Subtract_Point.contains(cursor_x, cursor_y)){
      Subtract_Point.drawButton(false);
 /*     for(int i=0; i< Points_num; i++){
        Serial.print(X_points[i]);
        Serial.print("   ");
        Serial.print(Y_points[i]);
        Serial.print(",   ");
      }
      Serial.println(Currently_Selected_Point);
  */    if (Currently_Selected_Point != 0xffff){
        Undraw_Point(Currently_Selected_Point);
        Regression_Model->Delete_Point_At(Currently_Selected_Point);
        Currently_Selected_Point = 0xffff;
        Undraw_Point_Position();
      }
      Subtract_Point.drawButton(true);
    }
    else if (Calculate_B.contains(cursor_x, cursor_y)){
      Calculate_B.drawButton(false);
      if(Currently_Selected_Point != 0xffff) Deselect_Point();
      Regression_Model->Calculate_Polynomial_Coefficients();
      Recalculate_Chart_Scale_Wash_Screen();
      Draw_Polynomial_Equation();
      Draw_Axis();
      Draw_Polynomial();
      Draw_Points();
      Draw_Buttons();
    }
    else if (Arrow_Right_B.contains(cursor_x, cursor_y) && (Arrow_Right_B.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Arrow_Right_B.drawButton(false);
      Tft->setTextColor(PRGUI_BLACK);
      Tft->setTextSize(2);
      Tft->setCursor(Top_Left_X+Width-92, Top_Left_Y+R_Menu_Bot-CALCULAE_B_HEIGHT-19);
      Tft->print(Coefficients_Option_str);
      if (Regression_Model->Coefficients_Auto_Calculate_Num_Settings < 0xde38){
        Regression_Model->Coefficients_Auto_Calculate_Num_Settings++;
        Coefficients_Option_str = String(Regression_Model->Coefficients_Auto_Calculate_Num_Settings);
     }
     else if (Regression_Model->Coefficients_Auto_Calculate_Num_Settings > 0xde39){
       Regression_Model->Coefficients_Auto_Calculate_Num_Settings--;
       uint16_t tmp = Regression_Model->Coefficients_Auto_Calculate_Num_Settings - 0xde38;
       Coefficients_Option_str = "lsqr" + String(8*tmp);
     }
     else { //if Coefficients_Auto_Calculate_Num_Settings == 0xde39
       Regression_Model->Coefficients_Auto_Calculate_Num_Settings = 0;
       Coefficients_Option_str = String(Regression_Model->Coefficients_Auto_Calculate_Num_Settings);
     }
      Tft->setTextColor(PRGUI_CYAN);
      Tft->setTextSize(2);
      Tft->setCursor(Top_Left_X+Width-92, Top_Left_Y+R_Menu_Bot-CALCULAE_B_HEIGHT-19);
      Tft->print(Coefficients_Option_str);
      Arrow_Right_B.drawButton(true);
    }
    else if (Arrow_Left_B.contains(cursor_x, cursor_y) && (Arrow_Left_B.Last_Time_Used_millis + 200 < Get_Current_Time_millis())){
      Arrow_Left_B.drawButton(false);
      Tft->setTextColor(PRGUI_BLACK);
      Tft->setTextSize(2);
      Tft->setCursor(Top_Left_X+Width-92, Top_Left_Y+R_Menu_Bot-CALCULAE_B_HEIGHT-19);
      Tft->print(Coefficients_Option_str);
      if (Regression_Model->Coefficients_Auto_Calculate_Num_Settings >= 0xde39){
        Regression_Model->Coefficients_Auto_Calculate_Num_Settings++;
        uint16_t tmp = Regression_Model->Coefficients_Auto_Calculate_Num_Settings - 0xde38;
        Coefficients_Option_str = "lsqr" + String(8*tmp);
     }
     else if (Regression_Model->Coefficients_Auto_Calculate_Num_Settings == 0){
       Regression_Model->Coefficients_Auto_Calculate_Num_Settings = 0xde39;
       uint16_t tmp = Regression_Model->Coefficients_Auto_Calculate_Num_Settings - 0xde38;
       Coefficients_Option_str = "lsqr" + String(8*tmp);
     }
     else {
       Regression_Model->Coefficients_Auto_Calculate_Num_Settings--;
       Coefficients_Option_str = String(Regression_Model->Coefficients_Auto_Calculate_Num_Settings);
     }
      Tft->setTextColor(PRGUI_CYAN);
      Tft->setTextSize(2);
      Tft->setCursor(Top_Left_X+Width-92, Top_Left_Y+R_Menu_Bot-CALCULAE_B_HEIGHT-19);
      Tft->print(Coefficients_Option_str);
      Arrow_Left_B.drawButton(true);
    }
    else Cursor_Check_Point_Selection(cursor_x, cursor_y);
  }
  void Draw_Chart_Gui(){
    Draw_Polynomial_Equation();
    Draw_Axis();
    Draw_Polynomial();
    Draw_Points();
    Draw_Buttons();
  }
};
