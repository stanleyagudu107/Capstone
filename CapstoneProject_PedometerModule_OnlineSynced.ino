#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PulseSensorPlayground.h>

Adafruit_MPU6050 mpu;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite screenprep = TFT_eSprite(&tft);
const int PulseWire = 32;    
int Threshold = 550;  
PulseSensorPlayground pulseSensor; 


//float alpha = -59.3954;
//float beta = 0.6334;
//double gamma = 0.0024;

uint8_t broadcastAddress[] = {0xA8, 0x42, 0xE3, 0x3D, 0x63, 0xD4};

typedef struct struct_message {
    int msghr;
    int msgsc;
    int msgee;
} struct_message;

typedef struct struct_rcvd {
    int rxaq;
    int rxhg;
    int rxsg;
    int rxeg;
    int rxht;
    int rxwt;
    int rxage;
} struct_rcvd;

struct_message pedometerdata;
struct_rcvd airqdata;
esp_now_peer_info_t peerInfo;

int last_step_rate_time = 0;
int last_pulse_measured = 0;
const int sample_array_length = 50;
int steps_taken = 0;
const int period = 20;
int window_size = 0;
int step_rate = 0;
int max_oldx = -10; int min_oldx = -10;
float x_acc, y_acc, z_acc = 0;
float abs_vals[sample_array_length] = {};
int data_point = 0;
float abs_val = 0;
float potential_steps = 0;
bool past_data_available = false;
int time_pushed = 0;
int toggle_screen_btn = 0;
int start_stop_btn = 35;
int prev_screen_on = 0;
int en_push = 1;
int seconds_angle = 0;
int screen_on = 0;
int hours = 0; int minutes = 0; int seconds = 0; 
int current = 0; int previous = 0;
int target_step_rate = 150;
int minutes_coordinates[2] = {10,65};
int seconds_coordinates[2] = {85,50};
int hours_coordinates[2] = {10,35};
int airqlabel_coordinates[2] = {68, 117};
int airqbar_coordinates[2] = {10,135};
int airqtxt_coordinates[2] = {68, 150};
int calslabel_coordinates[2] = {68, 175};
int cals_coordinates[2] = {10, 191};
int calstxt_coordinates[2] = {68, 206};
int minutes_fontsize = 4;
int seconds_fontsize = 4;
int hours_fontsize = 4;
int heart_rate = 69;
float energy_burned = 0;
int airq = 42;
int target_heart_rate = 120;
int step_goal = 10000;
int energy_goal = 500;
int airq_max = 300;
int stepsbar_len, airqbar_len, energybar_len = 0; int stepsbar_lenB, airqbar_lenB, energybar_lenB = 0;
int spx = 2; int spy = 2; int uw = 4; int uh = 8; int us = 2; 
int cell_colour = TFT_RED;
int hub_angle = 60;
int weight = 0;
int height = 0;
int age = 22;
int tx_colour = TFT_RED;
int time_circle_vanished = 0;
int transmitting = 0;
int previous_tx = 0;

void setup() {
  // put your setup code here, to run once:  
  setup_mpu6050();
  setupPulseSensor();
  setup_peerToPeerComms();
  pinMode(toggle_screen_btn, INPUT_PULLUP);
  pinMode(start_stop_btn, INPUT_PULLUP);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  screenprep.createSprite(135, 240);
  energy_burned = 0;
  displayScreenA();

}


void loop() {
  // put your main code here, to run repeatedly:
  doStepCounting();
  current = millis();
  if (current - previous_tx >= 12000) transmitData();
  if (current - last_pulse_measured >= 800) measurePulse(); 
  if ((current - previous) >= 1000){
    //energy_burned += (((alpha + (beta*heart_rate) + (gamma*step_rate*60))/60));
    previous = current;
    
    increaseTime();
    displayScreen(screen_on);


  }

  

  if(en_push){
    screen_on = checkPushButtons(screen_on); time_pushed = millis();
  }

  if ((current - time_pushed) >= 1000) {
    if (en_push == 0){
      time_pushed = current;
      en_push = 1;
    }
  }
  
}


void displayScreenA(){

  screenprep.fillScreen(TFT_BLACK);

  if (transmitting){
    transmitting = 0; clearDot(); 
  }
  if (current - time_circle_vanished >= 800) displayDot();
  //battery
  for (int xx = 1; xx<7;xx++){
    screenprep.fillRect(spx,spy,uw,uh, cell_colour);
    spx += (uw + us);
    switch (xx){
      case 1: cell_colour = TFT_RED; break;
      case 2: cell_colour = TFT_RED; break;
      case 3: cell_colour = TFT_YELLOW; break;
      case 4: cell_colour = TFT_YELLOW; break;
      case 5: cell_colour = TFT_GREEN; break;
      default: cell_colour = TFT_GREEN; break;
    }
  }
  spx = 2; spy = 2; uw = 4; uh = 8; us = 2; 
  cell_colour = TFT_RED;  

  //time
  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);
  screenprep.drawString(timeString(hours,minutes), 85 , 2, 2);

  screenprep.drawArc(68, 80, 57, 52, 50, 310, TFT_SILVER, TFT_SILVER);
  screenprep.drawArc(68, 80, 48, 43, 50, 310, TFT_SILVER, TFT_SILVER);

  
  if (heart_rate < target_heart_rate) {
    hub_angle = ((260*heart_rate)/(target_heart_rate)) + 50;
    screenprep.drawArc(68, 80, 57, 52, 50, hub_angle, TFT_DARKGREEN, TFT_DARKGREEN);
  }
  else{
    screenprep.drawArc(68, 80, 57, 52, 50, 310, TFT_DARKGREEN, TFT_DARKGREEN);
  } 

  if (step_rate < target_step_rate) {
    hub_angle = ((260*step_rate)/(target_step_rate)) + 50;
    screenprep.drawArc(68, 80, 48, 43, 50, hub_angle, TFT_DARKGREEN, TFT_DARKGREEN);
  }
  else{
    screenprep.drawArc(68, 80, 48, 43, 50, 310, TFT_DARKGREEN, TFT_DARKGREEN);
  } 


  screenprep.drawString(String(target_heart_rate), 25, 118, 2);
  screenprep.drawString(String(target_step_rate), 90, 118, 2);

  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);

  screenprep.drawCentreString(String(heart_rate), 68, 60, 4);
  screenprep.drawCentreString(String(step_rate), 68, 90, 4);

  //data
  screenprep.fillRect(0, 145, 135, 10, TFT_DARKGREEN);
  screenprep.setTextColor(TFT_WHITE);
  screenprep.drawString("DATA",2,146,1);

  if (steps_taken < step_goal) stepsbar_len = ((steps_taken*129)/step_goal); else stepsbar_len = 129;
  if (energy_burned < energy_goal) energybar_len = ((energy_burned*129)/energy_goal); else energybar_len = 129;
  if (airq < airq_max) airqbar_len = ((airq*129)/airq_max); else airqbar_len = 129;

  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);
  screenprep.drawString("Steps: "+ String(steps_taken), 2,163,2);
  screenprep.fillRect(2, 182, 129, 3, TFT_SILVER);
  screenprep.fillRect(2, 182, stepsbar_len, 3, TFT_GREEN);
  screenprep.drawString("Energy: "+ String(energy_burned)+" kcal", 2,190,2);
  screenprep.fillRect(2, 210, 129, 3, TFT_SILVER);
  screenprep.fillRect(2, 210, energybar_len, 3, TFT_GREEN);
  screenprep.drawString("AirQ: "+ String(airq), 2,217,2);
  screenprep.fillRect(2,234, 129, 3, TFT_SILVER);
  screenprep.fillRect(2,234, airqbar_len, 3, TFT_GREEN);

  screenprep.pushSprite(0,0);

}


String timeString(int hr, int min){
  String time_str = padNumber(hr) + " : " + padNumber(min);
  return time_str;

}

void increaseTime(){
  if (seconds < 59) seconds += 1;
  else 
    //minutes_changed = 1;
    if (minutes < 59) { minutes += 1; seconds = 0;}
    else
      //hours_changed = 1;
      if (hours < 23) {hours += 1; minutes = 0; seconds = 0;}
      else {hours = 0; minutes = 0; seconds = 0;}
}


void displayScreenB(){

  screenprep.fillScreen(TFT_BLACK);

  if (transmitting){
    transmitting = 0; clearDot(); 
  }
  if (current - time_circle_vanished >= 800) displayDot();

  for (int xx = 1; xx<7;xx++){
    screenprep.fillRect(spx,spy,uw,uh, cell_colour);
    spx += (uw + us);
    switch (xx){
      case 1: cell_colour = TFT_RED; break;
      case 2: cell_colour = TFT_RED; break;
      case 3: cell_colour = TFT_YELLOW; break;
      case 4: cell_colour = TFT_YELLOW; break;
      case 5: cell_colour = TFT_GREEN; break;
      default: cell_colour = TFT_GREEN; break;
    }
  }
  spx = 2; spy = 2; uw = 4; uh = 8; us = 2; 
  cell_colour = TFT_RED;  


  if (seconds == 0) { 
    screenprep.fillCircle(85, 60, 36, TFT_BLACK);
    
  }
    
  else{
    seconds_angle = angleLogic(180 + 6*seconds);
    screenprep.drawArc(85, 60, 35, 30, 180, seconds_angle, TFT_GREEN, TFT_GREEN);
  }

  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);
  screenprep.drawCentreString(padNumber(seconds), seconds_coordinates[0] , seconds_coordinates[1], seconds_fontsize);
  screenprep.drawString(padNumber(minutes), minutes_coordinates[0] , minutes_coordinates[1], minutes_fontsize);
  screenprep.drawString(padNumber(hours), hours_coordinates[0] , hours_coordinates[1], hours_fontsize);
 
  screenprep.drawRoundRect(5, 30, 40, 65, 5, TFT_DARKGREEN);
  screenprep.drawRoundRect(5, 105, 120, 130, 5, TFT_DARKGREEN);

  screenprep.drawCentreString("AirQ", airqlabel_coordinates[0], airqlabel_coordinates[1], 2);
  screenprep.drawRect(airqbar_coordinates[0], airqbar_coordinates[1], 110, 15, TFT_GREEN);
  screenprep.drawCentreString(String(airq), airqtxt_coordinates[0], airqtxt_coordinates[1], 2);

  screenprep.drawCentreString("Energy", calslabel_coordinates[0], calslabel_coordinates[1], 2);
  screenprep.drawRect(cals_coordinates[0], cals_coordinates[1], 110, 15, TFT_GREEN);
  screenprep.drawCentreString(String(energy_burned), calstxt_coordinates[0], calstxt_coordinates[1], 2);

  if (energy_burned < energy_goal) energybar_lenB = ((energy_burned*110)/energy_goal); else energybar_lenB = 110;
  if (airq < airq_max) airqbar_lenB = ((airq*110)/airq_max); else airqbar_lenB = 110;

  screenprep.fillRect(airqbar_coordinates[0], airqbar_coordinates[1], airqbar_lenB, 15, TFT_GREEN);
  screenprep.fillRect(cals_coordinates[0], cals_coordinates[1], energybar_lenB, 15, TFT_GREEN);

  screenprep.pushSprite(0,0);

}


String padNumber(int n){
  if (n/10 == 0) return ("0" + String(n));
  else return (String(n));
}


void displayScreen(int n){
  switch(n){
    case 1: displayScreenB(); break;
    default: displayScreenA(); break;
  }
}


int angleLogic(int n){
  if (n > 360) return (n%360);
  else return (n);
}


int checkPushButtons(int n){
  if (digitalRead(toggle_screen_btn) == 0) {en_push = 0; return n^=1;}
  else return n;
}


void setup_mpu6050() {
  // Try to initialize!
  if (!mpu.begin()) {
    // Print loading on screen maybe?
    while (1) {
      delay(10);
    }
  }
  // Print complete or something
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // 5, 10, 21, 44, 94, 184, 260
  delay(100);
}


bool isGreater(float val_a, float val_b) {
  return (val_a - val_b) > 0.5;
}

int getIndex(int n) {
  if (n >= sample_array_length) return n % sample_array_length;
  else if (n >= 0) return n;
  else return sample_array_length + n;
}

void lpfAvg(int m) {
  if (!past_data_available) {
    abs_vals[m] = abs_val;
  } else {
    abs_vals[m] = (abs_val + abs_vals[getIndex(m - 1)] + abs_vals[getIndex(m - 2)] + abs_vals[getIndex(m - 3)]) / 4;
  }
}

void countSteps() {
  int num_of_peak_pairs = 0;
  int consecutiveness = 0;
  float sensitivity = 0.25;
  int tmax = 0;
  int tmin = 0;
  int potential_steps_int = 0;
  max_oldx = -10;
  min_oldx = -10;

  float threshold_vals[sample_array_length] = {};
  float peak_max_vals[sample_array_length] = {};
  float peak_min_vals[sample_array_length] = {};
  float peak_max_vals_normalized[sample_array_length] = {};
  float peak_min_vals_normalized[sample_array_length] = {};

  for (int x = 1; x < sample_array_length; x++) {
    if (isGreater(abs_vals[x], abs_vals[getIndex(x + 1)]) && isGreater(abs_vals[x], abs_vals[getIndex(x - 1)])) {
      if (x - max_oldx >= 10){
        peak_max_vals[tmax] = abs_vals[x];
        max_oldx = x;
        tmax += 1;
      }
      
    }
    if (isGreater(abs_vals[getIndex(x + 1)], abs_vals[x]) && isGreater(abs_vals[getIndex(x - 1)], abs_vals[x])) {
      if (x - min_oldx >= 10){
        peak_min_vals[tmin] = abs_vals[x];
        tmin += 1;
        min_oldx = x;
      }
    }
  }

  if (tmax >= 1 && tmin >= 1) {
    int diff = tmax - tmin;
    if (diff > 0) {
      for (int y = 0; y < (tmax - diff); y++) {
        peak_max_vals_normalized[y] = peak_max_vals[y];
        peak_min_vals_normalized[y] = peak_min_vals[y];
        num_of_peak_pairs = y;
      }
    } else if (diff < 0) {
      for (int z = 0; z < (tmin + diff); z++) {
        peak_min_vals_normalized[z] = peak_min_vals[z];
        peak_max_vals_normalized[z] = peak_max_vals[z];
        num_of_peak_pairs = z;
      }
    } else {
      for (int z = 0; z < tmin; z++) {
        peak_min_vals_normalized[z] = peak_min_vals[z];
      }
      for (int z = 0; z < tmax; z++) {
        peak_max_vals_normalized[z] = peak_max_vals[z];
        num_of_peak_pairs = z;
      }
    }

    for (int zz = 0; zz <= num_of_peak_pairs; zz++) {
      threshold_vals[zz] = (peak_max_vals_normalized[zz] + peak_min_vals_normalized[zz]) / 2;
    }

    for (int yy = 0; yy <= num_of_peak_pairs; yy++) {
      if (peak_max_vals_normalized[yy] > (threshold_vals[yy] + sensitivity)) potential_steps += 0.5;
      if (peak_min_vals_normalized[yy] < (threshold_vals[yy] - sensitivity)) potential_steps += 0.5;
    }
  }

  potential_steps_int = potential_steps;
  steps_taken += potential_steps_int; potential_steps = 0;
  if ((millis() - last_step_rate_time) >= 6000){
    step_rate = (potential_steps_int*60);
    last_step_rate_time = millis();
  }
/*
  if (!consecutiveness) {
    if (potential_steps >= 8) {
      consecutiveness = 1;
      steps_taken += potential_steps_int;
      potential_steps = 0;
    }
  } else {
    steps_taken += potential_steps_int;
    if (potential_steps == 0) consecutiveness = 0;
    potential_steps = 0;
  }
*/

}


void doStepCounting(){
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  x_acc = a.acceleration.x;
  y_acc = a.acceleration.y;
  z_acc = a.acceleration.z;
  abs_val = abs(x_acc) + abs(y_acc) + abs(z_acc);

  window_size += 1;
  if (window_size >= 4) past_data_available = true;
  lpfAvg(data_point);
  if (window_size >= sample_array_length) {
    countSteps();
    window_size = 0;
  }

  delay(period);
  data_point = (data_point + 1) % sample_array_length;
}


void displayDot(){
  screenprep.fillCircle(68, 10, 6, tx_colour);
}


void clearDot(){
    screenprep.fillCircle(68,10,6,TFT_BLACK);
    time_circle_vanished = millis();
}


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == 0) tx_colour = TFT_GREEN;
  else tx_colour = TFT_RED;

}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&airqdata, incomingData, sizeof(airqdata));
  airq = airqdata.rxaq;
  step_goal = airqdata.rxsg;
  target_heart_rate = airqdata.rxhg;
  energy_goal = airqdata.rxeg;
  height = airqdata.rxht;
  weight = airqdata.rxwt;
  age = airqdata.rxage;
}


void setup_peerToPeerComms() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    delay(10);
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;     
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    delay(10);
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}


void transmitData() {
 
  pedometerdata.msghr = heart_rate;
  pedometerdata.msgsc = steps_taken;
  pedometerdata.msgee = energy_burned;

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &pedometerdata, sizeof(pedometerdata));   
  transmitting = 1;
  if (result == ESP_OK) tx_colour = TFT_GREEN;
  else tx_colour = TFT_RED;
  previous_tx = millis();
}


void setupPulseSensor() {     
  pulseSensor.analogInput(PulseWire);   
  pulseSensor.setThreshold(Threshold);   
 
  if (pulseSensor.begin()) {
    tft.fillCircle(68,80,40,TFT_GREEN);   
  }
}


void measurePulse(){

  if (pulseSensor.sawStartOfBeat()) {        
    heart_rate = (heart_rate + pulseSensor.getBeatsPerMinute()) >> 1;  
    if (heart_rate > 120 || heart_rate < 55) heart_rate = 75;                     
  }
  last_pulse_measured = millis();                   
}
