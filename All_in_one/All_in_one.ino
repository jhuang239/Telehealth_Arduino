//libraries
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <dht.h>
#include <MAX30100_PulseOximeter.h>

//define oled display
#define OLED_I2C_ADDR 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // SSD1306 doesn't have a reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//define dht pin and variable
#define dht_pin A7 //dht pin
dht DHT;

//define max30100 sensor variable and time period
#define REPORTING_PERIOD_MS     1000
MAX30100 sensor;
PulseOximeter pox;
uint32_t tsLastReading = 0;

//user health data
int blood_pressure_high;
int blood_pressure_low;
float temperature;
int blood_oxygen;
float heartbeat;
float room_temperature;
float humidity;
int ecgArr[128];

//received message from ESP32
String message = "";

//blood_pressure monitor
int val_hb = 0;// blood pressure signal
int val_bp = 0;// average blood pressure

//int blood_high;
//int blood_low;
int cnt = 0;

//ecg
int a = 0;
int lasta = 0;
int lastb = 0;

//configuration for max30100 module
void configureMax30100() {
  sensor.setMode(MAX30100_MODE_SPO2_HR);
  sensor.setLedsCurrent(MAX30100_LED_CURR_50MA, MAX30100_LED_CURR_27_1MA);
  sensor.setLedsPulseWidth(MAX30100_SPC_PW_1600US_16BITS);
  sensor.setSamplingRate(MAX30100_SAMPRATE_100HZ);
  sensor.setHighresModeEnabled(true);
}

//max30100 temperature setup
void temp_max30100_setup() {
  if (!sensor.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }
  configureMax30100();
}

//max30100 hr and spo2 setup
void hr_spo2_max30100_setup() {
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }
}

//setup oled
void setupOLED(void) {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) { // Address 0x3C for SSD1306 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) yield(); // Don't proceed, loop forever
  }
}

//oled setup for DHT11
void oled_DHT() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  display.cp437(true);
  display.println("DHT measuring");
  display.setCursor(0, 24);
  display.print("Temperature:       C");
  display.setCursor(0, 50);
  display.print("Humidity:          %");
  display.display();
  delay(500);
}

//oled setup for blood pressure
void oled_blood_pressure() {
  display.clearDisplay();
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(WHITE, BLACK); // Draw white text

  display.setCursor(0, 0);
  display.cp437(true); // Use full 256 char 'Code Page 437' font
  display.print("Blood Pressure Mon.");
  display.setCursor(0, 16); // Start at top-left corner
  display.print("RT_BP  :         mmHg");

  display.setCursor(0, 32);
  display.print("SYS_BP :         mmHg");

  display.setCursor(0, 48);
  display.print("DIA_BP :         mmHg");

  display.display();
  delay(2000);
}

//oled setup for welcom message
void oled_welcome() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  display.cp437(true);
  display.println("Welcome to");
  display.println("  Health");
  display.println(" Companion");
  display.display();
  delay(2000);
}



//display instruction of how to use ECG module
void oled_ecg_instruction() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println("Instructions for ECG:");
  display.setCursor(0, 30);
  display.println("1. Attach the Red Pad to your Right Hand ");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("2. Attach the Green  Pad to your Left Hand");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("3.Attach the Yellow  Pad to your Right    Thigh.");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("4.Press button B to capture the ECG graph when you feel stable");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("5.If you understand  the instructions,   press button A to    start.");
  display.display();
}

//oled display instruction of blood pressure monitor
void oled_blood_pressure_instruction() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("Instructions:");
  display.setCursor(0, 30);
  display.println("1.Press Air pump     Button.");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("2.When the Real-Time pressure reaches 130mmHg,stop pressing Air  Pump Button.");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("3.Wait the blood pressure to reduce until amplitude reaches maximum and turns steady");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("4.Press Button A to  record the Systolic  Blood Pressure.");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 20);
  display.println("5.Continue to observe the amplitude until it reduces to around half the the peak    value");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("6.Press Button B to  record the Diastolic Blood Pressure.");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("7.If you understand the instruction, Press button A to start");
  display.display();
}

//oled display instruction of dht11 and max30100
void oled_section(String section) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);
  if (section == "DHT") {
    display.println("Measuring home temperature and humidity");
    display.display();
    delay(3000);
  } else if (section == "MAX30100_temp") {
    display.println("1.Place your finger on the sensor.");
    display.print("2.Press button A to measure body temperature");
    display.display();
    delay(3000);
  } else if (section == "MAX30100_hr") {
    display.println("1.Place your finger on the sensor.");
    display.print("2.Press button A to measure heart rate");
    display.display();
    delay(3000);
  }
  else {
    display.println("1.Place your finger on the sensor.");
    display.print("2.Press button A to measure spo2");
    display.display();
    delay(3000);
  }
}

//oled display max30100 measurement.
void oled_max30100() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  display.cp437(true);
  display.println("MAX30100 measuring");
  display.display();
}

//read data from DHT11
void read_DHT() {
  oled_DHT();
  room_temperature = 0;
  humidity = 0;
  int count = 0;
  uint32_t starttime = millis();
  while ((millis() - starttime) <= 30000) {
    DHT.read11(dht_pin);
    display.setCursor(74, 24);
    display.print(DHT.temperature);
    display.setCursor(70, 50);
    display.print(DHT.humidity);
    display.display();
    room_temperature += DHT.temperature;
    humidity += DHT.humidity;
    count++;
  }
  room_temperature = room_temperature / count;
  humidity = humidity / count;
}

//read spo2 value from max30100
void read_max30100_spo2() {
  hr_spo2_max30100_setup();
  oled_max30100();
  blood_oxygen = 0;
  uint32_t starttime = millis();
  int count = 0;
  while ((millis() - starttime) <= 30000) {
    pox.update();
    if (millis() - tsLastReading > REPORTING_PERIOD_MS) {
      int value = pox.getSpO2();
      display.setCursor(0, 40);
      display.print("Spo2:          %");
      display.setCursor(70, 40);
      display.print(value);
      display.display();
      if (value <= 100 && value >= 90) {
        blood_oxygen += value;
        count++;
      }
      Serial.print("SpO2:");
      Serial.print(pox.getSpO2());
      Serial.println("%");

      tsLastReading = millis();
    }
  }
  blood_oxygen = blood_oxygen / count;
  pox.shutdown();
}

//read heart rate from max30100
void read_max30100_hr() {
  hr_spo2_max30100_setup();
  oled_max30100();
  heartbeat = 0.0;
  uint32_t starttime = millis();
  int count = 0;
  while ((millis() - starttime) <= 30000) {
    pox.update();
    if (millis() - tsLastReading > REPORTING_PERIOD_MS) {
      float value = pox.getHeartRate();
      heartbeat += value;
      count++;
      display.setCursor(0, 40);
      display.print("Heart rate:      bpm");
      display.setCursor(70, 40);
      display.print(value);
      display.display();
      tsLastReading = millis();

      Serial.print("BPM: ");
      Serial.println(value);
    }
  }
  heartbeat = heartbeat / count;
  pox.shutdown();
}

//read body temperature from max30100
void read_max30100_temp() {
  temp_max30100_setup();
  oled_max30100();
  temperature = 36.5;
  uint32_t starttime = millis();
  int count = 0;
  while ((millis() - starttime) <= 30000) {
    sensor.update();
    if (millis() - tsLastReading > REPORTING_PERIOD_MS) {
      sensor.startTemperatureSampling();
      if (sensor.isTemperatureReady()) {
        float temp = sensor.retrieveTemperature();
        display.setCursor(0, 40);
        display.print("Temperature:       C");
        display.setCursor(74, 40);
        display.print(temp);
        display.display();
        Serial.print("Temperature = ");
        Serial.print(temp);
        Serial.println("*C | ");

        tsLastReading = millis();
        if (temp > 35.00) {
          temperature += temp;
          count++;
        }
      }
    }
  }
  temperature = temperature / (count + 1);
  sensor.shutdown();
}

//unpack the json message from ESP32
void unpack(String jsonMessage) {
  StaticJsonDocument<96> doc;
  DeserializationError error = deserializeJson(doc, jsonMessage);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  String command = doc["command"];
  String username = doc["username"];
  if (command == "start_meausre") {
    oled_section("DHT");
    read_DHT();
    Serial.print("Temperature: ");
    Serial.println(room_temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);

    oled_blood_pressure_instruction();
    while (digitalRead(13) != LOW) {
      //break the loop after user press the push button A
    }
    blood_pressure();
    Serial.print("bph: ");
    Serial.println(blood_pressure_high);
    Serial.print("bpl: ");
    Serial.println(blood_pressure_low);
    delay(3000);

    oled_section("MAX30100_temp");
    while (digitalRead(13) != LOW) {
      //break the loop after user press the push button A
    }
    read_max30100_temp();
    Serial.print("Temperature: ");
    Serial.print(temperature);

    oled_section("MAX30100_spo2");
    while (digitalRead(13) != LOW) {
      //break the loop after user press the push button A
    }
    read_max30100_spo2();
    Serial.print("Blood Oxygen: ");
    Serial.print(blood_oxygen);


    oled_section("MAX30100_hr");
    while (digitalRead(13) != LOW) {
      //break the loop after user press the push button A
    }
    read_max30100_hr();
    Serial.print("Hear Rate: ");
    Serial.print(heartbeat);

    oled_ecg_instruction();
    while (digitalRead(13) != LOW) {
      //break the loop after user press the push button A
    }
    ecg_measuremnt();

    Serial.print("ECG: ");
    for (int i = 0; i < 128; i++) {
      Serial.print(ecgArr[i]);
      Serial.print(", ");
    }

    String packJson = packData(username);
    delay(3000);
    Serial.println(packJson);
    delay(3000);
    Serial1.flush();
    Serial1.println(packJson);
    message = "";
    oled_welcome();
  }
}

//measure the ecg
void ecg_measuremnt() {
  display.clearDisplay();
  int i = 0;
  boolean clicked = false;
  while (i < 128) {
    if (digitalRead(12) == LOW) {
      clicked = true;
    }
    if (a > 127)
    {
      display.clearDisplay();
      a = 0;
      lasta = a;
    }
    while ((digitalRead(9) == 1) || (digitalRead(8) == 1)) {
      Serial.println("please stick the tips well on your body");
    }
    int value = analogRead(A5);
    if (clicked) {
      ecgArr[i] = value;
      i++;
    }
    //Serial.println(value);
    int b = 60 - (value / 16);
    display.setCursor(0, 0);
    display.writeLine(lasta, lastb, a, b, WHITE);
    lastb = b;
    lasta = a;
    display.writeFillRect(0, 55, 128, 16, BLACK);
    display.setCursor(0, 57);
    display.print("Value:   ");
    display.print(value);
    display.display();
    a++;
    delay(100);
  }
}

//measure the blood pressure
void blood_pressure() {
  oled_blood_pressure();
  blood_pressure_high = 0;
  blood_pressure_low = 0;
  while (blood_pressure_high == 0 || blood_pressure_low == 0) {
    // measuare the voltage HB-o/p of Pump Control Circuit
    val_hb = analogRead(A0);
    //Serial.println(val_hb);
    cnt = (cnt + 1) % 20;

    // measure the voltage BP-o/p of the PSM
    // and convert it into corresponding pressure in mmHg
    // BP(mmHg) = 80.0064*DVBP*Vcc/1023 - 24.0019;
    // DVBP: digital value of the pressure sensor output voltage sampled by analogRead
    // Vcc: 5V
    // The above formula is based on pressure sensor sensitivity chart
    val_bp = int((80.0064 * 5 * analogRead(A1) / 1023) - 24.0019);

    // avoid negative readings
    if (val_bp <= 0) {
      val_bp = 0;
    }

    // update realtime blood pressure every 10 samples interval
    if ((cnt % 10) == 0) {
      display.fillRect(50, 16, 45, 8, BLACK);
      display.setTextColor(WHITE, BLACK); // Draw white text
      display.setCursor(50, 16);
      display.print(String(val_bp, DEC));
      display.display();
      Serial.print("current Blood pressure: ");
      Serial.println(val_bp);
    }

    // record the current blood pressure as Sys Blood Pressure
    // when Button A is pressed. Reading shows on OLED
    int SYS_RB = digitalRead(13);

    if (digitalRead(13) == LOW) {
      display.fillRect(50, 32, 45, 8, BLACK);
      display.setTextColor(WHITE, BLACK); // Draw white text
      display.setCursor(50, 32);
      display.print(String(val_bp, DEC));
      display.display();
      blood_pressure_high = val_bp;
    }

    // record the current blood pressure as Dia Blood Pressure
    // when Button B is pressed. Reading shows on OLED
    int DIA_RB = digitalRead(12);
    if (digitalRead(12) == LOW) {
      display.fillRect(50, 48, 45, 8, BLACK);
      display.setTextColor(WHITE, BLACK); // Draw white text
      display.setCursor(50, 48);
      display.print(String(val_bp, DEC));
      display.display();
      blood_pressure_low = val_bp;
    }
    delay(20);
  }
}

//pack all the measurred data into json object
String packData(String username) {
  Serial.println("packing");
  String jsonMessage;

  StaticJsonDocument<1536> obj;
  obj["command"] = "upload";
  obj["username"] = username;
  obj["blood_pressure_high"] = blood_pressure_high;
  obj["blood_pressure_low"] = blood_pressure_low;
  obj["temperature"] = temperature;
  obj["blood_oxygen"] = blood_oxygen;
  obj["heartbeat"] = heartbeat;
  obj["room_temperature"] = room_temperature;
  obj["humidity"] = humidity;
  JsonArray ecg = obj.createNestedArray("ecgData");
  for (int i = 0; i < 128; i++) {
    Serial.print("packing ecgData: ");
    Serial.println(i + 1);
    ecg.add(ecgArr[i]);
  }
  serializeJson(obj, jsonMessage);
  return jsonMessage;
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);

  pinMode(12, INPUT_PULLUP); //Button B record Diastolic pressure
  pinMode(13, INPUT_PULLUP); //Button A -record Systolic pressure

  //ecg
  pinMode(9, INPUT); //+
  pinMode(8, INPUT); //-

  setupOLED();
  oled_welcome();
}

void loop() {
  while (Serial1.available() > 0) {
    message = Serial1.readString();
    if (message.length() > 0) {
      Serial.println(message);
      unpack(message);
    }
  }
}
