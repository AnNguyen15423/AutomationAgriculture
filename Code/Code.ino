#define BLYNK_TEMPLATE_ID "TMPL6a9A3RhqT"
#define BLYNK_TEMPLATE_NAME "Terrarium"
#define BLYNK_AUTH_TOKEN "dKpvA9AyM4PUmLN2izu1JO6oCxy8hKek"

#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Stepper.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

char auth[] = "dKpvA9AyM4PUmLN2izu1JO6oCxy8hKek";
char ssid[] = "Yiang";
char pass[] = "thanhan154";

#define WaterLevelSensor 32
#define TempHumidSensor 25
#define SoilMoistureSensor 35
#define Pump 14
#define RainSensor 33
#define ON_button 17
#define OFF_button 16
#define HT 0
#define LocRem_button 5
#define Pump_button 4
#define VarRes 34


DHT dht(TempHumidSensor, DHT11);
Stepper stepMotor = Stepper(2048, 26, 12, 27, 13);
BlynkTimer timer;

bool LocRem = false;
int prestate = LOW;
bool stateBlynkON = false;
bool stateBlynkOFF = false;
bool statePump;
int prestatePump = LOW;
bool pumpOut = false;

int StepMBlynk;
int preValue = 0;
bool stateHT;

int waterlevel,WLpercents;
void sendWaterLevelData() {
  waterlevel= analogRead(WaterLevelSensor);
  WLpercents = map(waterlevel, 0, 4095, 0, 100);
  Blynk.virtualWrite(V0, WLpercents);
}

float humid,temp;
void sendHumidityAndTemperatureData() {
  humid = dht.readHumidity();
  temp = dht.readTemperature();
  Blynk.virtualWrite(V1, humid);
  Blynk.virtualWrite(V2, temp);

}

int SoilMois,SMpercents;
void sendSoilMoistureData() {
  SoilMois = analogRead(SoilMoistureSensor);
  SMpercents = 100 - map(SoilMois, 0, 4095, 0, 100);
  Blynk.virtualWrite(V3, SMpercents);
}

int isRaining,Raining;
void sendRainingState(){
  Raining = analogRead(RainSensor);
  if((100 - map(Raining, 0, 4095, 0, 100))>10){
    isRaining = 1;
  }
  else{
    isRaining = 0;
  }
  Serial.println(Raining);
  Serial.println(isRaining);

  //Blynk.virtualWrite(V8, isRaining);
}

int state;
bool LocRem_state() {
  state = digitalRead(LocRem_button);

  if (state != prestate) {
    if (state == LOW) {
      LocRem = !LocRem;
    }
    delay(10);
  }

  prestate = state;
  Blynk.virtualWrite(V9,LocRem);
  return LocRem;
}

BLYNK_WRITE(V9) {
  LocRem = param.asInt();
}

BLYNK_WRITE(V4) {
  StepMBlynk = param.asInt();
}


BLYNK_WRITE(V5) {
  statePump = param.asInt();
}

bool automation = false;
BLYNK_WRITE(V8) {
  automation = param.asInt();
}

int value,mapped_value;
bool thresholdCrossed = false;
void StepMotor(){
  if(automation == 0){
    if(!LocRem_state()){
      stepMotor.step(10*(StepMBlynk - preValue)); 
      preValue = StepMBlynk;
    }
    else if(LocRem_state()){
      value = analogRead(VarRes);
      mapped_value = map(value, 0, 4095,0, 2048);
      if(abs(mapped_value-preValue)>200){
        stepMotor.step(10*(mapped_value - preValue)); 
        preValue = mapped_value;
      }
      else{
        preValue = mapped_value;
      }
    }
  }

  else if (automation == 1) {
    if (isRaining == 0) {
      if (temp > 30 && humid > 70) {
        if (!thresholdCrossed1) {
          stepMotor.step(10 * (2048 - preValue));
          preValue = 2048;
          thresholdCrossed1 = true;
        }
        else{
          if(temp<30&&humid<70){
            thresholdCrossed1 = false;
          }
        }
      } 
      else if (humid > 90) {
        if (!thresholdCrossed2) {
          stepMotor.step(10 * (0 - preValue));
          preValue = 0;
          thresholdCrossed2 = true;
        }
        else{
          if(humid<90){
            thresholdCrossed2 = false;
          }
        }
      }
    } 
    else if (isRaining == 1) {
      if (!thresholdCrossed) {
        stepMotor.step(10 * (0 - preValue));
        preValue = 0;
        thresholdCrossed = true;
      }
      if (SMpercents > 30) {
        if (!thresholdCrossed) {
          stepMotor.step(10 * (2048 - preValue));
          preValue = 2048;
          thresholdCrossed = true;
        }
      }
    }
    if (thresholdCrossed) {
    // After executing the stepMotor.step, reset the thresholdCrossed flag
      thresholdCrossed = false;
    }
  }

  Blynk.virtualWrite(V4, preValue);

  if (thresholdCrossed) {
    // After executing the stepMotor.step, reset the thresholdCrossed flag
    thresholdCrossed = false;
  }
}


int PumpStateButton;
void controlPump(){
  if(automation == 0){
    if(!LocRem_state()){
      if(statePump ==1){
        pumpOut = HIGH;
      }
      else{
        pumpOut = LOW;
      }
    }

    else if(LocRem_state()){
      PumpStateButton = digitalRead(Pump_button);
      if (PumpStateButton != prestatePump) {
        if (PumpStateButton == LOW) {
          pumpOut = !pumpOut;
        }
        delay(10); // Chờ một khoảng thời gian ngắn để tránh đọc giá trị không chính xác do nhiễu
      }
      prestatePump = PumpStateButton;
    }
  }
  
  else if(automation == 1){
    if (isRaining == 0) {
      if (SMpercents<30) {
        pumpOut = HIGH;
      }
      else {
        pumpOut = LOW;
      }
    }
  }
  digitalWrite(Pump, pumpOut);
  Blynk.virtualWrite(V5,pumpOut);
}

BLYNK_WRITE(V6) {
  stateBlynkON = param.asInt();
}

BLYNK_WRITE(V7) {
  stateBlynkOFF = param.asInt();
}

int ON,OFF;
void controlHT() {
  ON = digitalRead(ON_button); 
  OFF = digitalRead(OFF_button);

  if ((ON == HIGH||stateBlynkON== HIGH) && (OFF == LOW||stateBlynkOFF == LOW)) {
    stateHT = HIGH; // Bật HT khi chỉ nhấn nút ON
  } 
  else if ((ON == LOW||stateBlynkON== LOW) && (OFF == HIGH||stateBlynkOFF == HIGH)) {
    stateHT = LOW; // Tắt HT khi chỉ nhấn nút OFF
  }

  digitalWrite(HT, stateHT); // Áp dụng trạng thái hiện tại vào chân HT
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  stepMotor.setSpeed(15);
  dht.begin();

  pinMode(ON_button, INPUT);
  pinMode(OFF_button, INPUT);
  pinMode(HT, OUTPUT);
  pinMode(Pump_button, INPUT);
  pinMode(LocRem_button, INPUT);
  pinMode(Pump, OUTPUT);
  pinMode(VarRes, INPUT);

  timer.setInterval(1010L, sendWaterLevelData);
  timer.setInterval(1510L, sendHumidityAndTemperatureData);
  timer.setInterval(2010L, sendSoilMoistureData);
  timer.setInterval(2510L, sendRainingState);

  lcd.init();
  lcd.backlight();
  lcd.clear();
}


void loop() {

  Blynk.run();
  timer.run();
  controlHT();

  if (stateHT == 1) {
    controlPump();
    StepMotor();
    //automationGarden();

    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temp);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Water level:");
    lcd.print(WLpercents);
    lcd.print("%");

    if (LocRem_state()) {
      lcd.setCursor(11, 0);
      lcd.print("  LOC");
    } else if (!LocRem_state()) {
      lcd.setCursor(11, 0);
      lcd.print("  REM");
    }
  } else if (stateHT == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Press ON button ");
    lcd.setCursor(0, 1);
    lcd.print("to run system   ");
  }

}
