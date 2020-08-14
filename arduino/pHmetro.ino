
/*************************************************************************************
  Original code by Kevin Lo, March 2015
  Modified by Julia Ferreira and Pedro Nascimento, October 2018
  This program will show PH on the LCD panel and send it on bluetooth serial port.
  
  Require Library :
  LiquidCrystal : http://arduino.cc/en/Reference/LiquidCrystal
  
  Version :
  v0.1 23/04/2019    Version Modified
**************************************************************************************/

#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

#define STX 0xAA                                // define STX for serial communication
#define ETX 0XBB                                // define ETX for serial communication

//Address I2C  MPU6050
const int MPU=0x68;  
//Variáveis para armazenar valores dos sensores (giroscópio, acelerômetro e temperatura)
float AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

//Pino CS do modulo cartao SD
const int chipSelect = 10;
File myFile;

RTC_DS1307 rtc;           //Modulo RTC DS1307

LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);   // Inicializa o display no endereco 0x27

byte RxCmd [4] = {0,0,0,0};

// define some values used by the panel and buttons
int lcd_key     = -1;
int adc_key_in  = 0;
int adc_key_prev  = -1;
int CurrentMode = 0;                           // 0 = Normal Display , 1 = Debug1 , 2 = Debug2
int CalSelect = 0;                             // 0 = PH4 Calibration Select , 1 = PH7 Calibration Select

const int NumReadings = 10;                    // number of reading
int Index = 0;                                 // index

int Ph1Readings[NumReadings];                  // array for store PH1 readings
int ph_ch1[NumReadings];
int Ph1Total = 0;                              // PH1 running total
int Ph1Average = 0;                            // PH1 average reading

int Ph2Readings[NumReadings];                  // array for store PH2 readings
int ph_ch2[NumReadings];
int Ph2Total = 0;                              // PH2 running total
int Ph2Average = 0;                            // PH2 average reading

double Ph7Buffer = 7;                          // For PH7 buffer solution's PH value , 7 or 6.86
double Ph4Buffer = 1;                          // For PH4 buffer solution's PH value , 4 or 4.01

double Ph7Ch1Reading = 272;                    // Ph7Ch1 Buffer Solution Reading.
double Ph7Ch2Reading = 265;                    // Ph7Ch2 Buffer Solution Reading.
double Ph4Ch1Reading = 443;                    // Ph4Ch1 Buffer Solution Reading.
double Ph4Ch2Reading = 455;                    // Ph4Ch2 Buffer Solution Reading.

double Ph1Ratio = 0;                           // PH1 Step
double Ph1Value = 0;                           // Ph1 Value in Human Reading Format after calculation

double Ph2Ratio = 0;                           // PH2 Step
double Ph2Value = 0;                           // Ph2 Value in Human Reading Format after calculation

long previousMillis = 0;                       // Variável de controle do tempo
long Interval = 4000;                          // Tempo em ms do intervalo a ser executado

boolean setDisplay = true;

//TESTE
int incomingByte = 0;
char buf[100];
int num = 0;
String dados;

int giro = 1;
int exame = 0;

int read_LCD_buttons(){                        // read the buttons
    adc_key_in = analogRead(0);                // read the value from the sensor 
    delay(10);                                 // switch debounce delay. Increase this delay if incorrect switch selections are returned.
    int k = (analogRead(0) - adc_key_in);      // gives the button a slight range to allow for a little contact resistance noise
    if (5 < abs(k)) return btnNONE;            // double checks the keypress. If the two readings are not equal +/-k value after debounce delay, it tries again.
    //lcd.print(adc_key_in);                   // read button value and print for calibrate 
    
    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result

    if (adc_key_in > 1000) return btnNONE; 
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 150)  return btnUP; 
    if (adc_key_in < 350)  return btnDOWN; 
    if (adc_key_in < 550)  return btnLEFT; 
    if (adc_key_in < 750)  return btnSELECT;  
    return btnNONE;                            // when all others fail, return this.
}

int reading(){                                 // Reading PH Data
  // Samplin PH Value     
  Ph1Total = Ph1Total - Ph1Readings[Index];    // subtract the last reading: 
  Ph2Total = Ph2Total - Ph2Readings[Index];      
  
  Ph1Readings[Index] = analogRead(1);          // read from the sensor : PH1
  Ph2Readings[Index] = analogRead(2);          // read from the sensor : PH2  
   
  Ph1Total = Ph1Total + Ph1Readings[Index];     // add the reading to the ph1 total:
  Ph2Total = Ph2Total + Ph2Readings[Index];     // add the reading to the ph2 total:
       
  Index = Index + 1;                           // advance to the next position in the array:                

  if (Index >= NumReadings){                   // if we're at the end of the array...         
    Index = 0;                                 // ...wrap around to the beginning:                         
    Ph1Average = Ph1Total / NumReadings;       // calculate the average1:
    Ph2Average = Ph2Total / NumReadings;       // calculate the average2:

    for(int i = 0; i<NumReadings; i++){
      ph_ch1[i] = (Ph7Ch1Reading - Ph1Readings[i]) / Ph1Ratio + Ph7Buffer;
      ph_ch2[i] = (Ph7Ch2Reading - Ph2Readings[i]) / Ph1Ratio + Ph7Buffer;
    }
    
  }

  Ph1Value = (Ph7Ch1Reading - Ph1Average) / Ph1Ratio + Ph7Buffer;    // Calculate PH ch1
  Ph2Value = (Ph7Ch2Reading - Ph2Average) / Ph2Ratio + Ph7Buffer;    // Calculate PH ch2

}

int send_ph_to_bt(){
  //Send to Bluetooth
  Serial.print("PH1: " + String(Ph1Value));
  Serial.print("; PH2: " + String(Ph2Value));
  Serial.print('\n');
}

void software_Reset(){ // Restarts program from beginning but does not reset the peripherals and registers
  asm volatile ("  jmp 0");
}  

void setup(){

   lcd.begin(16, 2);    // start LCD library
   Serial.begin(9600); // Serial Bluetooth (RX,TX)(17, 16) & Serial Monitor
   
//-----------Inicializa o MPU-6050-----------
   Wire.begin();
   Wire.beginTransmission(MPU);
   Wire.write(0x6B); 
   Wire.write(0); 
   Wire.endTransmission(true);
//-------------------------------------------

//----------Inicializa o Modulo SD Card--------------
   Serial.println("Inicializando SD card...");
   if (!SD.begin(chipSelect)) {
    Serial.println("Inicializacao falhou!");
    return;
   }
   Serial.println("Inicializacao finalizada.");
//---------------------------------------------------

//----------------Inicializa o RTC-------------------   
   if (! rtc.isrunning()) {
    Serial.println("RTC não está ligando!");
    // This will reflect the time that your sketch was compiled
    rtc.adjust(DateTime(__DATE__, __TIME__));
   }
    rtc.adjust(DateTime(__DATE__, __TIME__));
//---------------------------------------------------

//--------------Leitura da EEPROM--------------------
//   int ph4 = (EEPROM.read(4) * 256) + EEPROM.read(5);
//   Ph4Reading =  double(ph4);
//
//   int ph7 = (EEPROM.read(7) * 256) + EEPROM.read(8);
//   Ph7Reading = double(ph7);
//---------------------------------------------------

   for (int PhThisReading = 0; PhThisReading < NumReadings; PhThisReading++){        // initialize all the Ph readings to 0:
     Ph1Readings[PhThisReading] = 0;
     Ph2Readings[PhThisReading] = 0;
   }  
   
   Ph1Ratio = (Ph4Ch1Reading - Ph7Ch1Reading) / (Ph7Buffer - Ph4Buffer);                 // Calculate Ph1 Ratio   
   Ph2Ratio = (Ph4Ch2Reading - Ph7Ch2Reading) / (Ph7Buffer - Ph4Buffer);                 // Calculate Ph2 Ratio  

} //END void setup()
 

void loop(){ 

//---------------leitura do bluetooth do aplicativo-----------------------
//--------------paciente enquanto o exame esta ativo----------------------
  if(exame == 1){ 
      while(Serial.available() > 0){
          buf[num] = Serial.read();
          if (buf[num] == '\n'){
            String dados = buf;
            dados.trim();
            Serial.println(dados);
              myFile = SD.open("data.txt", FILE_WRITE);
              if (myFile) {
                myFile.println(dados);
                myFile.close();
              } else {
                Serial.println("Erro ao abrir data.txt");
              }
            // apaga o buffer
            for (int ca =0; ca<100; ca++)
            {
              buf[ca]=0;
            }
            num=0;
            break;
          }
          num++;
      }
  }
//------------------------------------------------------------------------


  unsigned long currentMillis = millis();       //Tempo atual em ms
  if (currentMillis - previousMillis > Interval && exame == 1) {  //Verificação do intervalo de 4s
      
      previousMillis = currentMillis;    // Salva o tempo atual
      
      myFile = SD.open("data.txt", FILE_WRITE);  //define 
  
    // if the file opened okay, write to it:
    if (myFile) {
      
      //Send to data.txt
      DateTime now = rtc.now();
      myFile.print(now.hour(), DEC);
      myFile.print(':');
      myFile.print(now.minute(), DEC);
      myFile.print(':');
      myFile.print(now.second(), DEC);
      myFile.print(";");
      myFile.print(Ph1Value);
      myFile.print(";");
      myFile.println(Ph2Value);
      
      //Send to Serial
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.print("; ");
      Serial.print(Ph1Value);
      Serial.print("; ");
      Serial.println(Ph2Value);

      // close the file data.txt:
      myFile.close();
      
    } else {
      // if the file didn't open, print an error:
      Serial.println("Erro ao abrir data.txt");
    }
    
//----------------------------Leitura do sensor MPU---------------------------------
    if(giro == 1){
      Wire.beginTransmission(MPU);
      Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
      Wire.endTransmission(false);
      //Solicita os dados do sensor
      Wire.requestFrom(MPU,14,true);  
      //Armazena o valor dos sensores nas variaveis correspondentes
      AcX=Wire.read()<<8|Wire.read(); //0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
      AcY=Wire.read()<<8|Wire.read(); //0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
      AcZ=Wire.read()<<8|Wire.read(); //0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
      Tmp=Wire.read()<<8|Wire.read(); //0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
      GyX=Wire.read()<<8|Wire.read(); //0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
      GyY=Wire.read()<<8|Wire.read(); //0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
      GyZ=Wire.read()<<8|Wire.read(); //0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

      //calculo para converter o valor bruto do acelerometro
      float Ax=AcX/16384;
      float Ay=AcY/16384;
      float Az=AcZ/16384;
      
      if (abs(AcX) > abs(AcY) && abs(AcX) > abs(AcZ) && AcX > 0) Serial.print("Em pé"); else Serial.print("Deitado");
  
      //Mostra os valores da aceleração na serial
      //Serial.print(" | AcX = "); Serial.print(Ax);
      //Serial.print(" | AcY = "); Serial.print(Ay);
      //Serial.println(" | AcZ = "); Serial.println(Az);
      
      delay(1);
    }
//---------------------------------END Giroscopio----------------------------------


  }//END if Verificação do intervalo de 4s


if (CurrentMode == 0)                        // Normal Display Mode
{
  reading();                                 // Reading PH Data for display 
  
  lcd.setCursor(3,0);                        // set the LCD cursor position 
  lcd.print("Ch1");
  lcd.setCursor(10,0); 
  lcd.print("Ch2");

  if(Ph1Value > 0 && Ph2Value > 0){
    lcd.setCursor(0,1);
    lcd.print("                ");
    
    // Display PH Data 
    lcd.setCursor(3,1);
    lcd.print(Ph1Value);                        // display PH1 value
       
    // Display PH Data 
    lcd.setCursor(10,1);
    lcd.print(Ph2Value);                        // display PH2 value
  }else{
    lcd.setCursor(0,1);
    lcd.print("                ");
    
    // Display PH Data 
    lcd.setCursor(3,1);
    lcd.print("PHx");                        // display PH1 value
       
    // Display PH Data 
    lcd.setCursor(10,1);
    lcd.print("PHx");                        // display PH2 value
  }
  delay(1);                                  // delay in between reads for stability   
}

if (CurrentMode == 1){
  reading();
  DateTime now = rtc.now();
  lcd.setCursor(8,0);
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  lcd.setCursor(0,1);
  lcd.print("PH   R");
  lcd.setCursor(10,1);
  lcd.print("P");
  lcd.setCursor(6,1);
  lcd.print(Ph1Average);
  lcd.setCursor(11,1);
  lcd.print(Ph1Value);
}

if (CurrentMode == 2){
  reading();
  double Ph1Voltage;
  Ph1Voltage = (double)Ph1Average * (5/10.24);

  double Ph2Voltage;
  Ph2Voltage = (double)Ph2Average * (5/10.24);
  
  lcd.setCursor(0,0);
  lcd.print("R:");
  lcd.setCursor(3,0);
  lcd.print(Ph1Average);
  
  lcd.setCursor(7,0);
  lcd.print("Ratio:");
  lcd.setCursor(13,0);
  lcd.print(Ph1Ratio);

  lcd.setCursor(0,1);
  lcd.print("PH:");
  lcd.setCursor(3,1);
  lcd.print(Ph1Value);    
}

if (CurrentMode == 3){          // Calibration Mode Selection Page
  lcd.setCursor(0,0);
  lcd.print("PH4 Cal ");
  lcd.setCursor(0,1);
  lcd.print("PH7 Cal ");
  if (CalSelect == 0) {
      lcd.setCursor(8,0);
      lcd.print(">>");
   }
  if (CalSelect == 1) {
      lcd.setCursor(8,1);
      lcd.print(">>");
   }
}

if (CurrentMode == 4){            // PH4 Calibration Mode
  reading();
  lcd.setCursor(0,0);
  lcd.print("PH4");
  lcd.setCursor(0,1);
  lcd.print("Cal");
  lcd.setCursor(4,0);
  lcd.print("C:");
  lcd.setCursor(7,0);
  lcd.print(Ph4Ch1Reading);
  lcd.setCursor(10,0);
  lcd.print("  ");
  lcd.setCursor(12,0);
  lcd.print(Ph4Ch2Reading);
  lcd.setCursor(15,0);
  lcd.print(" ");
  lcd.setCursor(4,1);
  lcd.print("R:");
  lcd.setCursor(7,1);
  lcd.print(Ph1Average);
  lcd.setCursor(12,1);
  lcd.print(Ph2Average);
}

if (CurrentMode == 5){            // PH7 Calibration Mode
  reading();
  lcd.setCursor(0,0);
  lcd.print("PH7");
  lcd.setCursor(0,1);
  lcd.print("Cal");
  lcd.setCursor(4,0);
  lcd.print("C:");
  lcd.setCursor(7,0);
  lcd.print(Ph7Ch1Reading);
  lcd.setCursor(10,0);
  lcd.print("  ");
  lcd.setCursor(12,0);
  lcd.print(Ph7Ch2Reading);
  lcd.setCursor(15,0);
  lcd.print(" ");
  lcd.setCursor(4,1);
  lcd.print("R:");
  lcd.setCursor(7,1);
  lcd.print(Ph1Average);
  lcd.setCursor(12,1);
  lcd.print(Ph2Average);
}

if (CurrentMode == 6 && exame == 0){            // Configurar
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Config. Exame");

  while(Serial.available() > 0){
    
    buf[num] = Serial.read();
    if (buf[num] == '\n'){
      String dados = buf;
      dados.trim();
      //Serial.println(dados);
      
      myFile = SD.open("data.txt", FILE_WRITE);
      if (myFile) {
        myFile.println(dados);
        myFile.close();
        exame = 1;
      } else {
        Serial.println("Erro ao abrir data.txt");
      }
      
      // apaga o buffer
      for (int ca =0; ca<100; ca++){buf[ca]=0;}
      num=0;
      break;
     }
    num++;
  }
  
  if(exame == 1){
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Configurado!");
        delay(3000);
        lcd.clear();
        CurrentMode = 0;
  }
  
}
   
   lcd.setCursor(0,1);             // move to the begining of the second line
   adc_key_prev = lcd_key ;       // Looking for changes

   lcd_key = read_LCD_buttons();   // read the buttons

  if (adc_key_prev != lcd_key)
  {
   //Serial.println("Key Press Change Detected");
   switch (lcd_key){               // depending on which button was pushed, we perform an action
       case btnRIGHT:{             //  push button "RIGHT" and show the word on the screen
            //lcd.print("RIGHT");
            if ( CurrentMode == 0 ){
               lcd.clear();
               CurrentMode = 2;
             }
            if ( CurrentMode == 3){
               lcd.clear();
               if ( CalSelect == 0 ){
                 CurrentMode = 4;
               } 
               if ( CalSelect == 1){
                 CurrentMode = 5;
               }
            }
            break;
       }
       case btnLEFT:{
             //lcd.print("LEFT "); //  push button "LEFT" and show the word on the screen
             if ( CurrentMode == 2 ){
               lcd.clear();
               CurrentMode = 0;
             }
             if ( CurrentMode == 3 ){
               lcd.clear();
               CurrentMode = 0;
             }
             if ( CurrentMode == 4 || CurrentMode == 5 ){
               lcd.clear();
               CurrentMode = 3;
             }

             break;
       }    
       case btnUP:{
             //lcd.print("UP   ");  //  push button "UP" and show the word on the screen
             if ( CurrentMode == 0 ){
               lcd.clear();
               CurrentMode = 1;
             }
             if ( CurrentMode == 3 ){
               lcd.clear();
               CalSelect = 0;
             }
             break;
       }
       case btnDOWN:{
             //lcd.print("DOWN ");  //  push button "DOWN" and show the word on the screen
             if (CurrentMode == 0){
               lcd.clear();
               //lcd.print("config");
               CurrentMode = 6;
             }
             if ( CurrentMode == 1){
               lcd.clear();
               CurrentMode = 0;
             }
             if ( CurrentMode == 3 ){
               lcd.clear();
               CalSelect = 1;
             }
             break;
       }
       case btnSELECT:{
             //lcd.print("SEL. ");  //  push button "SELECT" and show the word on the screen
             if ( CurrentMode == 0 ){
               lcd.clear();
               CurrentMode = 3;
               break;
             }
             if ( CurrentMode == 3 ){
               lcd.clear();
               CurrentMode = 0;
               break;
             }
             if ( CurrentMode == 4 ){ //calibrar ph4
               int ph4r = int(Ph1Average);
               EEPROM.write(4, ph4r/256);
               EEPROM.write(5, ph4r%256);
               lcd.clear();
               CurrentMode = 0;
               break;
             }
             if ( CurrentMode == 5 ){ //calibrar ph7
               int ph7r = int(Ph1Average);
               EEPROM.write(7, ph7r/256);
               EEPROM.write(8, ph7r%256);
               lcd.clear();
               CurrentMode = 0;
               break;
             }
               break;
       }
       case btnNONE:{
             //lcd.print("NONE ");  //  No action  will show "None" on the screen
             break;
       }
     }
   }
}
