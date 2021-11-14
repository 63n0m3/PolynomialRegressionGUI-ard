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

CH_TYPE Test_Get_Current_Sensor_Input(){
  return (CH_TYPE) analogRead(A15);
}

void loop() {
  Calibr_Device Sensor;
  ///At start you usually read from Eeprom location 
  if (Sensor.Load_From_Eeprom(50)) Serial.println("Eeprom read unsuccessfull");
  ///If screen is connected to the device, use it as follows:
  Chart_Gui Chart1(0, 0, tft.width(), tft.height(), 4, &tft);
  Chart1.Add_Regression_Model(&Sensor);
  Chart1.Recalculate_Chart_Scale_Wash_Screen();
  Chart1.Draw_Chart_Gui();
  Chart1.Set_Get_Current_Input_Fun(Test_Get_Current_Sensor_Input);    /// You may optionally send pointer to the function that reads current X value
  Serial.println (Sensor.Calculate_Polynomial_At(15.28f));          /// If you want to calculate the polynomial function value at X. Notice that GUI is not needed for it
  while(1){
    Cursor_Pressed = Touch_getXY();
    if (Cursor_Pressed){
  ///    tft.fillRect(Cursor_X-2, Cursor_Y-2, 5, 5, RED);                   /// <-------- Debug draws where cursor is pressed
      Chart1.Refresh_On_Cursor_Press(Cursor_X, Cursor_Y);

      
    }
  }
}
