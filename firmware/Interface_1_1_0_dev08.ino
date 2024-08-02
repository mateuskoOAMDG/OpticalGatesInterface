/// @brief Optical gates interface (firmware)
/// @version 1.1.0 (dev 08 beta)
/// @date 2024-01-27
/// @microcontroller:  Raspberry Pi Pico
/// @author (c) mateusko O.A.M.D.G, mateusko.OAMDG@outlook.com
/// @rev: 2024-07-24 - add: command-from-Serial; gates-count measurement


const int Led_Nothing = -1; //no action
const int Led_Off     = 0;  //LOW
const int Led_On      = 1;  //HIGH
const int Led_Blink   = 2;  //Blink On

// pins
const int ledY =  13;     //pin Yellow LED - connection
const int ledG =  10;     //pin Green LED  - measure
const int ledM1 = 6;      //pin Red LED - mode1
const int ledM2 = 2;      //pin Red LED -mode2

const int gate = 21;    //pin Signal: LOW - Gate In, HIGH - Gate Empty (on universal PCB - pin 22)
const int btnStart = 18;  //pin button Start/Stop
const int btnMenu = 27;   //pin button Menu

//menu number constants
const int Menu_Capture    = 0;  //menu: setings capture mode In/Out/In&Out
const int Menu_Autostart  = 1;  //menu: autostart Off/On
const int Menu_Buffered   = 2;  //menu: buffered capture Off/On

//capure mode constants
const int Mode_In = 0;
const int Mode_Out = 1;
const int Mode_Both = 2;

// buffer
const int MAX_DATA_BUFFER = 50; //Max. 25 Gates In/Out (or 50 Gates In only or Out only)

// Remote command-from-Serial
//
//     7      6      5      4      3      2      1      0 
//     OUT    IN    AUTO   BUFF   [G3]   [G2]   [G1]   [G0]                                   
//
//     IN     measure time on gate In
//     OUT    measure time on gate Out
//     AUTO   Autostart enabled
//     BUFF   Buffered measure ebabled
//     G3-G0  automatic buffered gates-count-autostart measurement 
//            0000 0 - off
//            0001 1 - 1 gate
//            0010 2 - 2 gates
//              ...
//            1111 15 - 15 gates
//
//            for Autorepeat must by AUTO - On and BUFF - On
//
//    



unsigned long data_buffer[MAX_DATA_BUFFER];
int buffer_index = 0; //pocet aktualnych hodnôt v buffri

//Long Clock Time
const unsigned long Long_Click_Time = 1000;

// ------------ Menu --------------
int menu = Menu_Capture;  // set active menu
int mode = Mode_In;       // capture mode 0-on In; 1- on Out; 2-on In&Out
bool buffered = false;    // True - send data after capturing; False - send data just now
bool autostart = true;    // True - capture autostart On; False - capture autostart Off
int gates = 0;            // 1 - 15 auto-count gates; 0 = off this feature

//------------------ Menu Leds -----------------------

int ledM1State = Led_Off;
int ledM2State = Led_Off;

unsigned long ledM_blink_timer;
bool  ledM_blink_state = false;
const unsigned long ledM_Blink_Time = 100ul; //miliseconds

// Nastavenie svietenia Menu Leds podla menu/value
void menuLedsSet(int menu, int value = 255) { //255 = no value
switch (menu) {
    case Menu_Capture:
      if (value == 255) {
        value = mode;
      }
      switch (value) {
        case Mode_In:
          ledM1State = Led_On;
          ledM2State = Led_Off;
          break;

        case Mode_Out:
          ledM1State = Led_Off;
          ledM2State = Led_On;
          break;

        case Mode_Both:
          ledM1State = Led_On;
          ledM2State = Led_On;
          break;
      }
      break;
    
    case Menu_Autostart:
      if (value == 255) {
        value = autostart;
      }
      if (value) {
        ledM1State = Led_Blink;
        ledM2State = Led_Blink;
        ledM_blink_timer = millis();
      } else {
        ledM1State = Led_Off;
        ledM2State = Led_Off;
      }
      break;

    case Menu_Buffered:
      if (value == 255) {
        value = buffered;
      }
      if (value) {
        ledM1State = Led_Blink;
        ledM2State = Led_Off;
        ledM_blink_timer = millis();
      } else {
        ledM1State = Led_Off;
        ledM2State = Led_Blink;
        ledM_blink_timer = millis();
      }
      break;
  }
  digitalWrite(ledM1, (ledM1State == Led_Off) ? LOW : HIGH);
  digitalWrite(ledM2, (ledM2State == Led_Off) ? LOW : HIGH);
} 

// Updatovanie svietenie Menu Leds (osetrenie blikania)
void menuLedsUpdate() {
  if ((ledM1State == Led_Blink) || (ledM2State == Led_Blink)) {
    if (millis() - ledM_blink_timer > ledM_Blink_Time) {
      ledM_blink_state = !ledM_blink_state;
      ledM_blink_timer = millis();
      if (ledM1State == Led_Blink)
        digitalWrite(ledM1, ledM_blink_state ? HIGH : LOW);
      if (ledM2State == Led_Blink)
        digitalWrite(ledM2, ledM_blink_state ? HIGH : LOW);
    }
  }
}
  
// ------------ Menu LEDs End -----------------

void setup() {

  pinMode(ledY, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledM1, OUTPUT);
  pinMode(ledM2, OUTPUT);
  
  pinMode(gate, INPUT_PULLUP);
  pinMode(btnStart, INPUT_PULLUP);
  pinMode(btnMenu, INPUT_PULLUP);


  // Initial settings 
  menu = Menu_Capture;        // active menu - 0 
  mode = Mode_In;    // capture only on "gate in"
  buffered = true; // buffered capturing On
  autostart = true; // autostart On
  buffer_index = 0; //buffer empty
  
  menuLedsSet( Menu_Capture, Mode_In);
  Serial.begin(115200);
}

int btnStartPressed() {
  static bool pressed = false;
  static bool previousState = true;
  static unsigned long timer = millis();
  static bool debouncing = false;

  bool state = digitalRead(btnStart);
  
  if (debouncing) {
    if (state == previousState) {
      if (millis() - timer > 20) {
          debouncing = false;
          return state ? -1 : 1;
      }
    } else {
      previousState = state;
      timer = millis();
    }
  } else {
    if (state == previousState) return 0;
    previousState = state;
    debouncing = true;
    timer = millis();
  }
  return 0;
}

//Obsluha tlacidla Menu
int btnMenuPressed() {
  static bool pressed = false;
  static bool previousState = true;
  static unsigned long timer = millis();
  static bool debouncing = false;

  bool state = digitalRead(btnMenu);
  
 if (debouncing) {
    if (state == previousState) {
      if (millis() - timer > 20) {
          debouncing = false;
          return state ? -1 : 1;
      }
    } else {
      previousState = state;
      timer = millis();
    }
  } else {
    if (state == previousState) {
      return 0;
    }
    previousState = state;
    debouncing = true;
    timer = millis();
  }
  return 0;
}
unsigned long longTimer;
int longPress() {
  static bool reading = false;
  static bool readed = false;
  switch (btnMenuPressed())  {
    case -1:
      reading = false;
      readed = false;
      return -1;
      break;

    case 1:
      longTimer = millis();
      reading = true;
      readed = false;
      return 1;
      break;

    case 0:
      if (reading && (digitalRead(btnMenu) == HIGH))  reading = false;
      if (reading && !readed && (millis() - longTimer > Long_Click_Time)) {
        readed = true;
        return 2;
      }
  }
  return 0;
}

// Testovacia procedura Serial
// Ak Serial nie je pripojeny blika zlta

void blinkIfNotSerial() {
  static bool serial_connected = false;
  static unsigned long timer = millis();
  static bool ledYon = false;
  unsigned long time = millis();
  if (Serial) {
    digitalWrite(ledY, HIGH);
    if (!serial_connected) {
      serial_connected = true;
      delay(100);
      Serial.println("#Optical_Gate_Interface v.1");
      sendMenu();
    }
    return;
  } else {
    serial_connected = false;
    if (time - timer > 50) {
      ledYon = !ledYon;
      digitalWrite(ledY, ledYon);
      timer = time; 
    }  
  }
}

// obsluha prepnutia menu
void nextMenu(bool changeMenu = false){ 
  int value = 0;   
  
  if (changeMenu) { //change menu
    menu ++;
    if (menu > 2) menu = 0;
    switch (menu) { //pri zmene menu sa nastavia ledky podla aktualnej hodnoty polozky menu
      case Menu_Capture: //0
        value = mode;
        break;
      
      case Menu_Autostart: //1
        value = autostart ? 1 : 0;
        break;

      case Menu_Buffered: //2
        value = buffered ? 1 : 0;
        break;
    }
  } else {  //change value
    switch (menu) {
      case Menu_Capture:
        mode++;
        if (mode > 2) mode = 0;
        value = mode;
        break;
      
      case Menu_Buffered:
        buffered = !buffered;
        value = buffered;
        break;

      case Menu_Autostart:
        autostart = !autostart;
        value = autostart;
        break;
    }
  }
  menuLedsSet(menu, value);
}

//obsluha menu tlacitkom Menu
void doMenu() {
  static bool chmenu = false;
  int ib = longPress();

  if (ib == 2) { //long press
    nextMenu(true); //change menu
    
    chmenu = true;
  }
  if (ib == -1) { //release short
    if (!chmenu) {
      nextMenu(false); //change value
    } else {
      chmenu = false;
    }
  }
  menuLedsUpdate();   
}

int signalOld = HIGH; // static for inline int signalOccured() 

//vracia 1 v okamihu vojdenia do brany; -1 v okamihu vyjdenia z brany; 0 - ziadna udalost
//1 a -1 vracia podla toho ak o je nastavene "mode"
inline int signalOccured() {
  int signal;
  signal = digitalRead(gate); //citanie stavu zbernice
  if (signal == signalOld) return 0;
  signalOld = signal;
  if ((mode != Mode_Out) && (signal == HIGH)) return 1; //IN
  if ((mode != Mode_In) && (signal == LOW)) return -1; //OUT
  return 0;
}

// Cyklus merania
// Input: mode, autostart, buffered, signal
// Output: Data to buffer, buffer_index | end to Serial
// return - true if stop-by-button; false  if stop by gates-count
bool measureCycle() {
  unsigned long timer; //zaciatok merania
  unsigned long time; //meranie casu udalosti vyskytu signalu
  unsigned long ledG_timer = millis();
  int gates_done = gates ;  //počet odmeranych bran
  int s; //signal: 1 In, -1 Out, 0 nothing
  bool stop_by_btn = true;

  if (mode == Mode_Both) gates_done *= 2;

  if (autostart) {
    // wait for signal occured
    while (signalOccured() == 0) {
      if (millis() - ledG_timer > 100ul) { //blikanie pocas cakania na 1. signal
        digitalWrite(ledG, !digitalRead(ledG));
        ledG_timer = millis();
      }
      //if (digitalRead(btnStart) == LOW) return; //stopped
      if (btnStartPressed() == 1) {
        stop_by_btn = true;
        buffer_index = -1;
        return stop_by_btn;
      }
    }
    data_buffer[0] = 0;
    gates_done --;
    buffer_index = 0; //buffer_index udava poslednu poziciu zapisanej hodnoty 
  } else {
    buffer_index = -1; //buffer je prazdny
  }
  digitalWrite(ledG, HIGH); //prebieha meranie - ledG svieti
  timer = micros(); //start time
  
  //prerusenie tlacidlom Stop
  while (btnStartPressed() != 1) {  
    s = signalOccured();
    
   
    if(s == 0) continue; //no signal
    
    time = micros(); //cas udalosti vyskytu signalu
    
    if (buffered) {
      data_buffer[++buffer_index] = time - timer;
     
         //odmerali sa vsetky brany
      if ((buffer_index) >= MAX_DATA_BUFFER - 1 ) break; //buffer full -> stop measuring
    } else {
      Serial.println(time - timer, DEC);
    }
     gates_done --;
     if (gates > 0 && gates_done == 0) {
        stop_by_btn = false;
        break;
     } 
  }
  return stop_by_btn;
}

//odoslanie buffra na Serial
void sendBuffer() {
  if ((mode == Mode_In) || (mode == Mode_Out)) { 
    for (int i = 0; i <= buffer_index; i++) {
      Serial.print(i, DEC);
      Serial.print(";");
      Serial.println(data_buffer[i], DEC);
    }
  } else {
    for (int i = 0; i <= buffer_index; i += 2) {
      Serial.print(i / 2, DEC);
      Serial.print(";");
      Serial.print(data_buffer[i], DEC);
      Serial.print(";");
      Serial.println(data_buffer[i + 1], DEC);
    }
  }
}

void sendCommand(const char* s) {
  Serial.println(s);
}

void sendMenu() {
  Serial.print("#");
  if (mode == Mode_In) Serial.print("I");
  else if (mode == Mode_Out) Serial.print("O");
  else if (mode == Mode_Both) Serial.print("IO");

  if (autostart)  Serial.print("A+"); else Serial.print("A-");

  if (buffered)  Serial.print("B+"); else Serial.print("B-");
   
  if (gates > 0) {
    Serial.print("G");
    Serial.print(gates, DEC);
  }   
   Serial.println("#");

 
}

unsigned long timer = millis();

////////////////////// Serial communication //////////////////////
//
// Serial protocol is {0x4E, command, verify, 0xFE}
//      verify = ~command
//
const unsigned int timeout = 500; // Serial communication timeout
byte  serial_command = 0;

// structure of bits of serial_command byte:
// 7     6     5     4     3     2     1     0  
// OUT   IN   AUTO   BUF   G3    G2    G1    G0

const int SERIAL_NO_DATA = 0;
const int SERIAL_TIMEOUT = 1;
const int SERIAL_NO_START_BYTE = 2;
const int SERIAL_READING = 3;
const int SERIAL_NO_END_BYTE = 4;
const int SERIAL_CHECK_ERROR = 5;
const int SERIAL_DATA_OK = 6;

// Read packet form Serial with Command, handles errors 
// Return: see consts above; ok = SERIAL_DATA_OK

int readSerial() {
  static unsigned int timer;
  static byte command = 0; // =? no command
  static int serial_buffer_index = 0;

  byte data; 
  unsigned int time = millis();

  if (Serial.available()) {
    data = Serial.read();  // continue after if()
  }
  else if (serial_buffer_index > 0 && time - timer > timeout) {
    serial_buffer_index = 0;
    return SERIAL_TIMEOUT;
  }
  else
    return SERIAL_NO_DATA;

  // here continue
  switch (serial_buffer_index) {
    case 0:
      if (data == 0x4E) {
        serial_buffer_index ++;
        timer = time;
      }
      else {
        serial_buffer_index = 0;
        return SERIAL_NO_START_BYTE;
      }
      return SERIAL_READING;
      break;

    case 1:
      command = data;
      serial_buffer_index ++;
      return SERIAL_READING;
      break;

    case 2:
      if ( data == byte(~command)) {
        serial_buffer_index ++;
        return SERIAL_READING;
      }
      else {
        //Serial.print("Command = "); Serial.println(command, HEX);
        //Serial.print("Data    = "); Serial.println(data,    HEX);
        serial_buffer_index = 0;
        return SERIAL_CHECK_ERROR; 
      }
      break;

    case 3:
      if (data == 0xFE) {
        serial_command = command;
        return SERIAL_DATA_OK; 
      }
      else {
        serial_buffer_index = 0; 
        return SERIAL_NO_END_BYTE;
      }
      break;
  }
  return 0xFF;
}

void proceedSerialCommand() {
    int result = readSerial();
    if (result == SERIAL_NO_DATA) return;
    if (result == SERIAL_DATA_OK) {
      
      Serial.print("#The command was received correctly: 0b");
      byte c = serial_command; for (int i = 0; i < 8; i++) { Serial.print(c & 0x80 ? '1' : '0'); c <<= 1;} //print BIN
      Serial.println();

      // set params by serial_command

      switch (serial_command >> 6) {
        case 0b01:
          mode = Mode_In;
          break;
        case 0b10:
          mode = Mode_Out;
          break;
        case 0b11:
          mode = Mode_Both;
          break;
        // 0b00 - no changed
      }

      autostart = serial_command & 0x20;
      buffered  = serial_command & 0x10;
      gates     = serial_command & 0x0F;

     // set menu leds by parameters
     
      if (gates > 0) buffered = true;

      menu = Menu_Capture;
      menuLedsSet(menu, mode);
      sendMenu();
    }
    else {
      //Serial.print("#Data error number: ");
      //Serial.println(result);

      
  

      
    }
}

bool autorepeat = false;
bool stop_by_button = false;

void loop() {
  blinkIfNotSerial();
  
  //čítanie a vykonanie príkazu zo Serial 
  proceedSerialCommand(); 
  autorepeat = buffered && autostart;
  //obsluha menu
  doMenu();
  
  //spustenie merania a odoslanie dát
  if (btnStartPressed() == 1)
    while(true) {
      if(Serial) {
        sendMenu(); 
        if(!buffered) sendCommand("#START#");
          
        stop_by_button = measureCycle(); //mesranie

        if (buffered) { //odoslanie dat
          sendCommand("#START#");
          sendBuffer();
          
          for(int i = 0; i < 3; i++) {     
            digitalWrite(ledG, HIGH);
            delay(300);
            digitalWrite(ledG, LOW);
            delay(50);

          } 
        } else {
          digitalWrite(ledG, LOW);
        }
        sendCommand("#STOP#");
      }

      //autorepeat condition
      if (Serial && autorepeat) {
        if (stop_by_button && !gates) {
          break; //cancel autorepeat
        }
      } 
      else
        break; //zber dát sa opakuje
    }
  
}
    
