#include <TFT_eSPI.h>
#include <SPI.h>
#include <sps30.h>
#include <esp_now.h>
#include <WiFi.h>

#define BACKLIGHT (0x3A98) //15000

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite screenprep = TFT_eSprite(&tft);

uint8_t broadcastAddress[] = {0x08, 0xD1, 0xF9, 0x6A, 0x71, 0x34};

typedef struct struct_rcvd {
    int msghr;
    int msgsc;
    int msgee;
} struct_rcvd;

typedef struct struct_message {
    int txaq;
    int txhg;
    int txsg;
    int txeg;
    int txht;
    int txwt;
    int txage;
} struct_message;

struct_message airqdata;
struct_rcvd pedometerdata;
esp_now_peer_info_t peerInfo;


float pm1_0mc = 10;
float pm2_5mc = 5;
float pm4_0mc = 11;
float pm10_0mc = 4;
float pm1_0nc = 14;
float pm2_5nc = 8;
float pm4_0nc = 6;
float pm10_0nc = 5;
float tps = 4.83;
int aqi = 47;
int temp = 29;
int humidity = 53;
int step_target = 7000;
int energy_target = 300;
int heart_target = 120;

int tx_colour = TFT_RED;
int time_circle_vanished = 0;
int toggle_screen_btn = 0;
int navigate_btn = 35;
int current_millis = 0;
int previous_millis = 0;
int tpsbarlen = 0;
int airq_max = 300;
int hub_angle = 0;
int co_conc = 200;
float entry = 0;
int bar_heights[10] = {0,0,0,0,0,0,0,0,0,0};
float pmmc_max = 30;
float pmnc_max = 60;
int tempbarlen = 0;
int humibarlen = 0;
int cobarlen = 0;
int whole_tps, dec_tps = 0;
int screen_on = 0;
int en_push = 1;
int time_pushed = 0;
int cursor_position = 6;
int bar_entry[6] = {0,0,0,0,0,0};
int place_values[6] = {100000,10000,1000,100,10,1};
int bar_cursor_position = 0;
int previous_tx = 0;

String setting_parameters[6] = {"Energy Goal","Step Goal","Heart Goal","Weight","Height","Age"};
int user_weight = 83;
int user_height = 178;
int user_age = 22;
int parameter_values[6] = {energy_target, step_target, heart_target, user_weight, user_height, user_age};
int field_colour = TFT_WHITE;
int uninterrupted_time = 0; 
int energy_saver_on = 0;

float pmtwofive[8] = {-0.1,12,35.4,55.4,150.4,250.4,350.4,500.4};//+0.1
int pmten[8] = {-1,54,154,254,354,424,504,604}; //+1
int aqi_range[8] = {-1,50,100,150,200,300,400,500};

float Iha, Ila,Cha,Cla,Conca = 0;
int Ihb, Ilb,Chb,Clb = 0; float Concb = 0;

int row1 = 33;    //13,12,26,27,32,33,25,17
int row2 = 25;
int row3 = 26;
int row4 = 27;
int column1 = 32;
int column2 = 12;
int column3 = 13;
int column4 = 17;
int btn_row = 0; int btn_column = 0;
int previous_keypad = 0;
int previous_lock = 0;
int prev_millis_aq = 0;

int key_pressed = 0;
int keypad_columns[4] = {column1, column2, column3, column4};
int which_col = 0;
int keypad_en = 1;

int incomingsc;
int incominghr;
int incomingee;

int transmitting = 0;


void setup() {
  // put your setup code here, to run once:
  //setupSPS30();
  setup_peerToPeerComms();
  tft.init();
  setupKeypad();
  pinMode(toggle_screen_btn, INPUT_PULLUP);
  pinMode(navigate_btn, INPUT_PULLUP);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  screenprep.createSprite(240, 135);
  displayMainScreen();

}

void loop() {
  // put your main code here, to run repeatedly:
  current_millis = millis();
  if (current_millis > 25000) aqi = 52;
  //if (current_millis - prev_millis_aq >= 1000){findAirQualityIndex(); prev_millis_aq = current_millis;}
  if (current_millis - previous_tx >= 12000) transmitData();
  if (!energy_saver_on){
    if ((current_millis - previous_millis) >= 400){
      previous_millis = current_millis;
      displayScreen(screen_on, cursor_position);
    }
    checkKeypadButtons();
    doActionWithPushedButton(buttonPushed(btn_row, btn_column), cursor_position);
  }

  if(en_push){
    screen_on = checkPushButtons(screen_on); time_pushed = millis();
  }

  if ((current_millis - time_pushed) >= 1000) {
    if (en_push == 0){
      time_pushed = current_millis;
      en_push = 1;
    }
  }

  if ((current_millis - uninterrupted_time) >= BACKLIGHT) {
    energy_saver_on = 1;
    displayScreenSaver();
  }
}


void displayMainScreen(){
  cursor_position = 6;
  screenprep.fillSprite(TFT_BLACK);
    if (transmitting){
    transmitting = 0; clearDot(); 
  }
  if (current_millis - time_circle_vanished >= 800) displayDot();
  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);

  screenprep.fillCircle(195, 45, 30, TFT_SILVER);
  screenprep.fillCircle(195, 45, 25, TFT_BLACK);
  screenprep.drawCentreString(String(aqi),195,35,4);
  hub_angle = angleLogic((180 + ((aqi*360)/airq_max)));
  screenprep.drawArc(195, 45, 30, 25, 180, hub_angle, TFT_GREEN, TFT_GREEN);

  entry = (pm1_0mc/pmmc_max)*65; bar_heights[0] = entry;
  entry = (pm2_5mc/pmmc_max)*65; bar_heights[1] = entry;
  entry = (pm4_0mc/pmmc_max)*65; bar_heights[2] = entry;
  entry = (pm10_0mc/pmmc_max)*65; bar_heights[3] = entry;
  entry = (pm1_0nc/pmnc_max)*65; bar_heights[6] = entry;
  entry = (pm2_5nc/pmnc_max)*65; bar_heights[7] = entry;
  entry = (pm4_0nc/pmnc_max)*65; bar_heights[8] = entry;
  entry = (pm10_0nc/pmnc_max)*65; bar_heights[9] = entry;
  for (int i = 0; i < 10; i++){
    if ((i!=4)&&(i!=5)) screenprep.fillRect(10 + i*8, 15 + 65 - bar_heights[i], 5, bar_heights[i], TFT_GREEN);
  }

  screenprep.drawRoundRect(105, 15, 45, 65, 5, TFT_GREEN);
  whole_tps = tps;
  entry = (tps - whole_tps)*100; dec_tps = entry;
  screenprep.drawCentreString(padNumber(whole_tps),128,25,4);
  screenprep.drawCentreString(padNumber(dec_tps),128, 51, 2);
  entry = (tps/10)*45; tpsbarlen = entry;
  screenprep.fillRect(105,75,tpsbarlen,5, TFT_GREEN);


  screenprep.drawRect(10, 90, 220, 41, TFT_GREEN);
  screenprep.drawFastVLine(80, 90, 40, TFT_GREEN);
  screenprep.drawFastVLine(160, 90, 40, TFT_GREEN);

  tempbarlen = temp+20;
  humibarlen = ((humidity*80)/100);
  cobarlen = ((co_conc*70)/500);

  screenprep.fillRect(10, 127, tempbarlen, 4, TFT_GREEN);
  screenprep.fillRect(80, 127, humibarlen, 4, TFT_GREEN);
  screenprep.fillRect(160, 127, cobarlen, 4, TFT_GREEN);

  screenprep.drawCentreString(String(temp),45,95,2);
  
  screenprep.drawCentreString(String(humidity) + "%",120,95,2);
  screenprep.drawCentreString(String(co_conc),195,95,2);

  screenprep.setTextColor(TFT_WHITE);
  screenprep.drawCentreString("ppm",195,107,2);
  screenprep.drawCentreString("degC",45,107,2);
  screenprep.drawCentreString("humidity",120,107,2);
  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);

  screenprep.pushSprite(0,0);
}


void displayConfigScreen(int n){
  
  screenprep.fillSprite(TFT_BLACK);

  if (transmitting){
    transmitting = 0; clearDot(); 
  }
  if (current_millis - time_circle_vanished >= 800) displayDot();

  screenprep.setTextColor(TFT_WHITE);
  for (int i = 0; i < 3; i++){
    field_colour = TFT_WHITE;
    screenprep.setTextColor(TFT_WHITE);
    if (n==i) {field_colour = TFT_GREEN; screenprep.setTextColor(TFT_GREEN);}
    screenprep.drawRect(10, 20 + i*42, 85, 20, field_colour);
    screenprep.drawString(setting_parameters[i], 11, 20 + i*42 - 12, 1);
  }

  for (int i = 0; i < 3; i++){
    field_colour = TFT_WHITE;
    screenprep.setTextColor(TFT_WHITE);
    if (n==(i+3)) {field_colour = TFT_GREEN; screenprep.setTextColor(TFT_GREEN);}
    screenprep.drawRect(135, 20 + i*42, 85, 20, field_colour);
    screenprep.drawString(setting_parameters[i + 3], 136, 20 + i*42 - 12, 1);  
  }
  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);

  for (int i = 0; i < 3; i++){
    if (n==i) screenprep.setTextColor(TFT_BLACK, TFT_WHITE);
    screenprep.drawString(String(parameter_values[i]), 13, 22 + i*42, 2);
    screenprep.setTextColor(TFT_WHITE, TFT_BLACK);
  }

  for (int i = 0; i < 3; i++){
    if (n==(i+3)) screenprep.setTextColor(TFT_BLACK, TFT_WHITE);
    screenprep.drawString(String(parameter_values[i+3]), 138, 22 + i*42, 2);
    screenprep.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  screenprep.pushSprite(0,0);
}


void displayScreenSaver(){
  screenprep.setTextColor(TFT_WHITE, TFT_BLACK);
  if (transmitting){
    transmitting = 0; clearDot(); 
  }
  if (current_millis - time_circle_vanished >= 800) displayDot();
  screenprep.fillSprite(TFT_BLACK);
  screenprep.fillCircle(120,68,40,TFT_SILVER);
  screenprep.fillCircle(120,68,35,TFT_BLACK);
  hub_angle = angleLogic((180 + ((aqi*360)/airq_max)));
  screenprep.drawArc(120, 68, 40, 35, 180, hub_angle, TFT_GREEN, TFT_GREEN);
  screenprep.drawCentreString(String(aqi), 120, 61, 4);

  screenprep.pushSprite(0,0);

}


int angleLogic(int n){
  if (n > 360) return (n%360);
  else return (n);
}


String padNumber(int n){
  if (n/10 == 0) return ("0" + String(n));
  else return (String(n));
}


int checkPushButtons(int n){
  if ((digitalRead(navigate_btn) == 0) && (screen_on == 1)) {
    en_push = 0;
    cursor_position = (cursor_position + 1) % 6;
    bar_cursor_position = 0;
    uninterrupted_time = millis(); 
    clearBarEntry();
    return n;
  }
  else if (digitalRead(toggle_screen_btn) == 0) {
    en_push = 0;
    uninterrupted_time = millis(); 
    energy_saver_on = 0;
    clearBarEntry();
    return n^=1;
  }
  else return n;
}


void displayScreen(int n, int pos){
  switch(n){
    case 1: displayConfigScreen(pos); break;
    default: displayMainScreen(); break;
  }
}


void enterBarFigure(int n){
  if (n == 10) backSpace();
  else {
    if (bar_cursor_position == 0) clearBarEntry();
    for (int i = 0; i<bar_cursor_position; i++){
      bar_entry[i + (6 - bar_cursor_position) - 1] = bar_entry[i + (6-bar_cursor_position)];
    }
    bar_entry[5] = n;
    bar_cursor_position = (bar_cursor_position + 1) % 6;
  }
}


void backSpace(){
  if (bar_cursor_position > 0){
    bar_cursor_position -= 1;
    for (int i = bar_cursor_position; i  > 0; i--){
      bar_entry[i] = bar_entry[i - 1];
    }
    bar_entry[0] = 0;
  }
}


void determineVal(int n){
  int val = 0;
  for (int i = 0; i<6; i++){
    val += (place_values[i]*bar_entry[i]);
  }
  if (n <= 5){
    switch(n){
      case 0: energy_target = val; parameter_values[n] = val; break; 
      case 1: step_target = val; parameter_values[n] = val; break;
      case 2: heart_target = val; parameter_values[n] = val; break;
      case 3: user_weight = val; parameter_values[n] = val; break;
      case 4: user_height = val; parameter_values[n] = val; break;
      case 5: user_age = val; parameter_values[n] = val; break;
    }
  }
}


void setupKeypad() {
  pinMode(row1, INPUT_PULLUP);
  pinMode(row2, INPUT_PULLUP);
  pinMode(row3, INPUT_PULLUP);
  pinMode(row4, INPUT_PULLUP);
  pinMode(column1, OUTPUT);
  pinMode(column2, OUTPUT);
  pinMode(column3, OUTPUT);
  pinMode(column4, OUTPUT);

}


void checkKeypadButtons(){
  if ((millis() - previous_keypad) >= 100){
    previous_keypad = millis();
    if (keypad_en) {
      digitalWrite(column1, HIGH); digitalWrite(column2, HIGH); digitalWrite(column3, HIGH); digitalWrite(column4, HIGH);
      digitalWrite(keypad_columns[which_col], LOW); 
      
      if (digitalRead(row1) == 0){
        btn_row = 1; btn_column = which_col + 1; keypad_en = 0; previous_lock = millis(); uninterrupted_time = previous_lock;
        key_pressed = 1;
      }
      if (digitalRead(row2) == 0){
        btn_row = 2; btn_column = which_col + 1; keypad_en = 0; previous_lock = millis(); uninterrupted_time = previous_lock;
        key_pressed = 1;
      }
      if (digitalRead(row3) == 0){
        btn_row = 3; btn_column = which_col + 1; keypad_en = 0; previous_lock = millis(); uninterrupted_time = previous_lock;
        key_pressed = 1;
      }
      if (digitalRead(row4) == 0){
        btn_row = 4; btn_column = which_col + 1; keypad_en = 0; previous_lock = millis(); uninterrupted_time = previous_lock;
        key_pressed = 1;
      }
      which_col = (which_col + 1) % 4;
    }
    if (((millis() - previous_lock) >= 500) && (!keypad_en)) keypad_en = 1;
  }
}


int buttonPushed(int r, int c){
  switch(r){
    case 1: switch(c){
              case 1: return 0;
              case 2: return 0;
              case 3: return 0;
              default: return 0;
            }
    case 2: switch(c){
              case 1: return 3;
              case 2: return 6;
              case 3: return 9;
              default: return 0;
            }
    case 3: switch(c){
              case 1: return 2;
              case 2: return 5;
              case 3: return 8;
              default: return 0;
            }
    case 4: switch(c){
              case 1: return 1;
              case 2: return 4;
              case 3: return 7;
              default: return 0;
            }
    default: return 0;
  }
}


void doActionWithPushedButton(int pb, int id){
  if (key_pressed && id<6){
    enterBarFigure(pb); determineVal(id); displayConfigScreen(id);
    key_pressed = 0;
  }
}


void clearBarEntry(){
  bar_entry[0] = 0;
  bar_entry[1] = 0;
  bar_entry[2] = 0;
  bar_entry[3] = 0;
  bar_entry[4] = 0;
  bar_entry[5] = 0;
}


void setupSPS30(){
  int16_t ret;
  uint8_t auto_clean_days = 4;
  uint32_t auto_clean;

  delay(2000);
  sensirion_i2c_init();

  while (sps30_probe() != 0) {
    delay(500);
  }

  ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
  ret = sps30_start_measurement();

  delay(3000);
}


int findBoundaries(float n, int xtype){
  if (n>0){
    if (xtype == 0){
      for(int i = 7; i>0; i--){
        if (n<pmtwofive[i]) return i;
      }
    }
    else{
      for(int i = 7; i>0; i--){
        if (n<pmten[i]) return i;
      }
    }
  }
}


int calcAirq(float pma, float pmb){//2.5,10
  //get bounds
  int a = findBoundaries(pma,0);
  int b = findBoundaries(pmb, 1);

  //Iha, Ila,Cha,Cla,Conca
  //Ihb, Ilb,Chb,Clb,Concb

  Cha = pmtwofive[a];
  Cla = pmtwofive[a-1]+0.1;
  Iha = aqi_range[a];
  Ila = aqi_range[a-1]+1;
  Conca = pma;

  Chb = pmten[b];
  Clb = pmten[b-1]+1;
  Ihb = aqi_range[b];
  Ilb = aqi_range[b-1]+1;
  Concb = pmb;

  float air_quality_index_a = (((Iha - Ila)/(Cha - Cla)) * (Conca - Cla)) + Ila;
  float air_quality_index_b = (((Ihb - Ilb)/(Chb - Clb)) * (Concb - Clb)) + Ilb;
  int aqa = air_quality_index_a;
  int aqb = air_quality_index_b;
  int air_quality_index = (aqa + aqb)>>1;

  return air_quality_index;

}


void findAirQualityIndex(){
  struct sps30_measurement m;
  char serial[SPS30_MAX_SERIAL_LEN];
  uint16_t data_ready;
  int16_t ret;
  
  do {
    ret = sps30_read_data_ready(&data_ready);
    if (ret < 0) {
      Serial.print("error reading data-ready flag: ");
      Serial.println(ret);
    } else if (!data_ready)
      Serial.print("data not ready, no new measurement available\n");
    else
      break;
    delay(100); /* retry in 100ms */
  } 
  
  while (1);

  ret = sps30_read_measurement(&m);
  if (ret < 0) {
    Serial.print("error reading measurement\n");
  } 
  else {

  #ifndef PLOTTER_FORMAT

    pm1_0mc = m.mc_1p0;
    pm2_5mc = m.mc_2p5;
    pm4_0mc = m.mc_4p0;
    pm10_0mc = m.mc_10p0;

  #ifndef SPS30_LIMITED_I2C_BUFFER_SIZE

    pm1_0nc = m.nc_1p0;
    pm2_5nc = m.nc_2p5;
    pm4_0nc = m.nc_4p0;
    pm10_0nc = m.nc_10p0;

    tps = m.typical_particle_size;

  #endif

  #else  

  Serial.println(" ");

  #endif /* PLOTTER_FORMAT */

  aqi = calcAirq(pm2_5mc, pm10_0mc);

  } 
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


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == 0) tx_colour = TFT_GREEN;
  else tx_colour = TFT_RED;

}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&pedometerdata, incomingData, sizeof(pedometerdata));
  incomingsc = pedometerdata.msgsc;
  incominghr = pedometerdata.msghr;
  incomingee = pedometerdata.msgee;

}


void transmitData() {
 
  airqdata.txaq = aqi;
  airqdata.txhg = heart_target;
  airqdata.txsg = step_target;
  airqdata.txeg = energy_target;
  airqdata.txht = user_height;
  airqdata.txwt = user_weight;
  airqdata.txage = user_age;
  

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &airqdata, sizeof(airqdata));   
  transmitting = 1;
  if (result == ESP_OK) tx_colour = TFT_GREEN;
  else tx_colour = TFT_RED;
  previous_tx = millis();
}


void displayDot(){
  screenprep.fillCircle(225, 10, 6, tx_colour);
}


void clearDot(){
    screenprep.fillCircle(225,10,6,TFT_BLACK);
    time_circle_vanished = millis();
}
