//find fontlist at https://github.com/olikraus/u8g2/wiki/fntlistall
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <BluetoothSerial.h>
#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial

ELM327 myELM327;

uint32_t rpm = 0;

#define ONBOARD_LED 2

// Pins on my ssd1306 are labeled as GND, VCC, D0, D1, RES, DC, CS
// clk = d0
// mosi = d1
// chip select = cs
// dc = data command pin
// res = reset pin

//use fonts here as "variables" for find and replace.  u8g2 setFont() can't use a variable - only a font name
//label font = u8g2_font_10x20_tf
//value font = u8g2_font_logisoso32_tf

#define D1   15
#define D0   2
#define DC   0
#define S1CS   14
#define S1RST  27
#define S2CS   26
#define S2RST  25
#define S3CS   13
#define S3RST  12

//after noname:
//f = full buffer - 1024 byte
//1 = page buffer - 128 byte
//2 = page buffer - 256 byte
U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI s1(U8G2_R0, D0, D1, S1CS, DC, S1RST);
U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI s2(U8G2_R0, D0, D1, S2CS, DC, S2RST);
U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI s3(U8G2_R0, D0, D1, S3CS, DC, S3RST);

//calculate half screen width for centering value
short sMidX = s1.getDisplayWidth() / 2;

String s1GaugeValue = "12.7";
String s1GaugeLabel = "Volts";
String s2GaugeValue = "30";
String s2GaugeLabel = "Oil Pressure";
String s3GaugeValue = "200";
String s3GaugeLabel = "Coolant Temp";

//each X value per screen needs its own calculated width due to varying strings.  Should be calculated in setup as it will not change, keep extra calculations out of main loop.
short s1GaugeLabelX = 0; //calculated center value based on pixel length of string displayed for screen 1's label value
short s2GaugeLabelX = 0;
short s3GaugeLabelX = 0;

short gaugeValY = 0;
short gaugeLabelY = 0;

short hLineY = 0;



void setup() {
  //BLUETOOTH BLOCK
#if LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  DEBUG_PORT.begin(115200);
  SerialBT.setPin("0000");
  ELM_PORT.begin("OBDII", true);

  if (!ELM_PORT.connect("OBDII"))
  {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
    while (1);
  }

  if (!myELM327.begin(ELM_PORT, true, 2000))
  {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    while (1);
  }

  Serial.println("Connected to ELM327");
  //SSD BLOCK
  //Serial.begin(9600);
  pinMode(ONBOARD_LED, OUTPUT);
  //  Serial.print("Hello World!");
  initializeScreens();
  getGaugeCoords();
}

void initializeScreens() {
  s1.begin();
  s1.setPowerSave(0);
  s2.begin();
  s2.setPowerSave(0);
  s3.begin();
  s3.setPowerSave(0);
}

void getGaugeCoords() {
  /*
      fonts set here once to get width calculation, so the calculation is just done once instead of each time in the loop
      value widths have to be calculated in loop as the displayed strings are not static.
      this allows for uniform centering regardless of font chosen
      if width of screen (s1.getDisplayWidth()) = 128, 128/2 = 64 = middle of screen
      if width of value (s1.getStrWidth()) = 32, 32/2 = 16 = middle of label
      64 - 16 = 48 = where the X value needs to be for the label to be centered
  */
  s1.setFont(u8g2_font_10x20_tf);  //set once here to get string width of
  s2.setFont(u8g2_font_10x20_tf);
  s3.setFont(u8g2_font_10x20_tf);

  s1GaugeLabelX =  sMidX - (s1.getStrWidth(s1GaugeLabel.c_str()) / 2);
  s2GaugeLabelX =  sMidX - (s2.getStrWidth(s2GaugeLabel.c_str()) / 2);
  s3GaugeLabelX =  sMidX - (s3.getStrWidth(s3GaugeLabel.c_str()) / 2);

  gaugeLabelY = s1.getDisplayHeight() + s1.getDescent(); //add the max descent of the label font to make sure it displays as far down on the screen as it can, without clipping any characters

  hLineY = s1.getDisplayHeight() - (s1.getMaxCharHeight() + 5); //position for horizontal line as max character height of gauge font + 5 pixels for padding

  s1.setFont(u8g2_font_logisoso32_tf); //set font to value font for calculating coordinates of value.  Only s1 is needed because only Y is calculated here
  gaugeValY = s1.getAscent();

}

void loop() {
  //BLUETOOTH BLOCK
  float tempRPM = myELM327.rpm();

  if (myELM327.nb_rx_state == ELM_SUCCESS)
  {
    rpm = (uint32_t)tempRPM;
    Serial.print("RPM: "); Serial.println(rpm);
  }
  else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    myELM327.printError();

  //SSD BLOCK

  //each X value per screen needs its own calculated width due to varying strings.  Should be calculated within the loop for values to maintain centering with changing values.
  short s1GaugeValX = 0; //calculated center value based on pixel length of string displayed for screen 1's gauge value
  short s2GaugeValX = 0;
  short s3GaugeValX = 0;

  s1.firstPage();
  do {
    //line 1
    s1.setFont(u8g2_font_logisoso32_tf);
    s1GaugeValX = sMidX - (s1.getStrWidth(s1GaugeValue.c_str()) / 2);
    s1.drawStr(s1GaugeValX, gaugeValY, s1GaugeValue.c_str());

    //line 2
    s1.setFont(u8g2_font_10x20_tf);
    s1.drawStr(s1GaugeLabelX, gaugeLabelY, s1GaugeLabel.c_str());
    s1.drawHLine(0, hLineY, 128);
  } while ( s1.nextPage() );

  s2.firstPage();
  do {
    s2.setFont(u8g2_font_logisoso32_tf);
    s2GaugeValX = sMidX - (s2.getStrWidth(s2GaugeValue.c_str()) / 2);
    s2.drawStr(s2GaugeValX, gaugeValY, s2GaugeValue.c_str());

    s2.setFont(u8g2_font_10x20_tf);
    s2.drawStr(s2GaugeLabelX, gaugeLabelY, s2GaugeLabel.c_str());
    s2.drawHLine(0, hLineY, 128);
  } while ( s2.nextPage() );

  s3.firstPage();
  do {
    s3.setFont(u8g2_font_logisoso32_tf);
    s3GaugeValX = sMidX - (s3.getStrWidth(s3GaugeValue.c_str()) / 2);
    s3.drawStr(s3GaugeValX , gaugeValY, s3GaugeValue.c_str());

    s3.setFont(u8g2_font_10x20_tf);
    s3.drawStr(s3GaugeLabelX, gaugeLabelY, s3GaugeLabel.c_str());
    s3.drawHLine(0, hLineY, 128);
  } while ( s3.nextPage() );

}
