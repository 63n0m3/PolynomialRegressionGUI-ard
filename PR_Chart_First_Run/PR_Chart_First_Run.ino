/*  discussion: https://www.avrfreaks.net/forum/tutdis-polynomial-regression-gui
 *  video tutorial: https://youtu.be/icBz3evB9uE
 *  by Gen0me, https://github.com/63n0m3/PolynomialRegressionGUI-ard    btc: bc1qn8xgw5rlfm7xnxnrxcqk0ulhrmrqjy07s3zmfp
*/
#include <EEPROM.h>
#include <MCUFRIEND_kbv.h>
#define CH_TYPE float
#include <PR_GUI_Ard1p0.hpp>
MCUFRIEND_kbv tft;
#include <TouchScreen.h>
#include <FreeDefaultFonts.h>
#define MINPRESSURE 200
#define MAXPRESSURE 1000
const int XP=8,XM=A2,YP=A3,YM=9; //240x320 ID=0x9595
//const int XP=6,XM=A2,YP=A1,YM=7; //320x480 ID=0x9486

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

int16_t Cursor_X, Cursor_Y;
bool Cursor_Pressed;
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
      Cursor_X = map(p.y, 94, 905, 0, tft.width());
      Cursor_Y = map(p.x, 129, 918, 0, tft.height());        
  //      Cursor_X = map(p.y, 963, 191, 0, tft.width());
  //      Cursor_Y = map(p.x, 185, 929, 0, tft.height());        
    }
    return pressed;
}

#define BLACK   0x0000

void setup() {
    Serial.begin(9600);
    uint16_t ID = tft.readID();
    tft.begin(ID);
    tft.setRotation(1);
    tft.fillScreen(BLACK);
    tft.setFont(NULL);
}

void loop() {
  ///During the first run you may need to run Save_To_Eeprom(..) function. Add at least 1 point before saving.  
  Calibr_Device Sensor;
  Sensor.Add_Point(8.0, 15.2);
  Sensor.Add_Point(12.0, 24.2);
  Sensor.Add_Point(15.0, 31.2);
  Sensor.Add_Point(19.0, 42.2);
  Sensor.Add_Point(22.0, 62.2);
//  Sensor.Calculate_Polynomial_Coefficients(5);
  Sensor.Calculate_Polynomial_Coefficients(Sensor.Find_Best_Fit_Coeff_Num(8));
  if (Sensor.Save_To_Eeprom(50, 200)) Serial.println("Eeprom write unsuccessfull");

  ///If screen is connected to the device, use it as follows:
  Chart_Gui Chart1(0, 0, tft.width(), tft.height(), 4, &tft);
  Chart1.Add_Regression_Model(&Sensor);
  Chart1.Recalculate_Chart_Scale_Wash_Screen();
  Chart1.Draw_Chart_Gui();
  while(1){
    Cursor_Pressed = Touch_getXY();
    if (Cursor_Pressed){
  ///    tft.fillRect(Cursor_X-2, Cursor_Y-2, 5, 5, RED);                   /// <-------- Debug draws where cursor is pressed
      Chart1.Refresh_On_Cursor_Press(Cursor_X, Cursor_Y);

      
    }
  }
}
