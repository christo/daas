#include <NewPing.h>
#include <EEPROM.h>

// TODO: go to specified height
// TODO: make movement detect when height is not changing - safety cutoff
// TODO: smooth out ultrasonic readings maybe use markov localization
// TODO: detect height change without movement (indicates obstacle)


// setting up serial on terminal?:
// cu -s 9600 -l /dev/tty.usbmodem1421

// pin assignments deliberately coincide with leostick blue and green leds, for up and down modes
// red pin is for stop
int redPin = 13;

int upPin = 10;
int downPin = 9;
// sonar
int triggerPin = 12;
int echoPin = 11;

// serial comand protocold
int up = byte('u'); 
int down = byte('d'); 
int stopit = byte('s'); 
int freeit = byte('f'); 
int incPing = byte('+');
int decPing = byte('-');
int setTop = byte('t');
int setBottom = byte('b');
int help = byte('?');


const int topMem = 0;
const int botMem = 1;
int dir = 0; // 0 = no movement, +ve = up, -ve = down
int currentDir = 0;
int baud = 9600;
int startTime = 0;
int runTime = 0;
int minTime = 1000;
int timeout = 20000; // enough to run, could be already at extreme
int pingInterval = 100; // whole main loop sleep delay
int minInterval = 80; 
int maxInterval = 2500;
const int pingSetSize = 8; 
int pingSet[pingSetSize]; // a ring buffer of pings
int pingIndex = 0; // next ping to write
int top = 116;
int bottom = 90;

boolean freeMode = false; // freemode means it will not stop at top/bottom, only timeout

NewPing sonar(triggerPin, echoPin);

void setup() {
  Serial.begin(baud); 
  pinMode(upPin, OUTPUT);
  pinMode(downPin, OUTPUT);
  pinMode(redPin, OUTPUT);  
  // intiialise the pingSet
  for(int i=0; i<pingSetSize; i++) {
    pingSet[i] = -1;
  }  
  // uninitialised EEPROM is 255, outside of max range of top/bottom
  int savedTop = EEPROM.read(topMem);
  if (savedTop != 255) {
    top = min(savedTop, 125); // max height
  }  
  int savedBottom = EEPROM.read(botMem);
  if (savedBottom != 255) {
    bottom = max(savedBottom, 72); // min height
  }
  
}

void loop() {
  readSerial();
  if (dir != currentDir) {
    // activate switch
    currentDir = dir; 
    Serial.print("setting direction to ");
    Serial.println(currentDir, DEC);

    if (currentDir == 1) {
      Serial.println("going up");
      digitalWrite(upPin, HIGH);
      digitalWrite(downPin, LOW);
      digitalWrite(redPin, LOW);
    } 
    else if (currentDir == -1) {
      Serial.println("going down");
      digitalWrite(downPin, HIGH);
      digitalWrite(upPin, LOW);
      digitalWrite(redPin, LOW);
    } else {
      stopDesk();
    }
    startTime = millis();
  }
  runTime = millis() - startTime;
  if (dir != 0 && runTime > timeout) {
    stopDesk();
    Serial.println("tripped timeout");
  }
  if (dir != 0 && !freeMode) {
    checkTopAndBottom(measure());
  }
  if (runTime % 200 == 0) {
    Serial.println("runTime: " + String(runTime));
    measure();
  }
  delay(pingInterval);
}

int measure() {
  pingSet[pingIndex] = sonar.ping_cm();
  return getAveragePingHeight();
}

/*
 * does a ping and gets the average of the past 10 pings
 */
int getAveragePingHeight() {
  
  int minPing = pingSet[pingIndex];
  int maxPing = minPing;
  int sum = 0;
  Serial.print("[");
  for (int i=0; i<pingSetSize; i++) {
    int p = pingSet[i];
    Serial.print(String(p) + " ");
    if (p == -1) {
      p = pingSet[pingIndex];
    }
    sum += p;
    maxPing = max(p, maxPing);
    minPing = min(p, minPing);
  }
  Serial.print("] avg excluding top+bottom: ");
  sum -= (maxPing + minPing);
  
  int height = sum / (pingSetSize - 2); // average of the 8 middle values

  Serial.println("height: " + String(height) + " ");  

  pingIndex++;
  if (pingIndex >= pingSetSize) {
    pingIndex = 0;
  }
  
  return height;
}

void checkTopAndBottom(int height) {
  // check if we have reached a set point
  if (height > top && dir == 1) {
    Serial.println("reached top " + String(height) + " cm");
    stopDesk();
  } else if (height < bottom && dir == -1) {
    Serial.println("reached bottom " + String(height) + " cm");
    stopDesk();
  }

}

void readSerial() { 
  if (Serial.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial.read();

    if (incomingByte == up) {
      dir = 1;
      Serial.println("up ");
    } else if (incomingByte == down) {
      dir = -1;
      Serial.println("down ");
    } else if (incomingByte == stopit) {
      dir = 0;
      Serial.println("stop ");
    } else if (incomingByte == freeit) {
      freeMode = !freeMode;
      Serial.println("freemode: " + String(freeMode));
    } else if (incomingByte == incPing) {      
      pingInterval = constrain(pingInterval+1, minInterval, maxInterval);
      Serial.println("ping interval ("+ String(minInterval) + " - " + String(maxInterval) +"): " + String(pingInterval));
    } else if (incomingByte == decPing) {      
      pingInterval = constrain(pingInterval-1, minInterval, maxInterval);
      Serial.println("ping interval ("+ String(minInterval) + " - " + String(maxInterval) +"): " + String(pingInterval));
    } else if (incomingByte == setTop) {
      top = max(getAveragePingHeight(), bottom);
      EEPROM.write(topMem, top);
      Serial.println("top set to: " + String(top));
    } else if (incomingByte == setBottom) {
      bottom = min(getAveragePingHeight(), top);
      EEPROM.write(botMem, bottom);
      Serial.println("bottom set to " + bottom);
    } else if (incomingByte == help) {
      printHelp();
    }
  }  
}

void printHelp() {
  Serial.println("u : up");
  Serial.println("d : down");
  Serial.println("s : stop");
  Serial.println("f : toggle free mode - when on, does no measurement or autostop");
  Serial.println("+ : increments the ping interval (ms)");
  Serial.println("- : decrements the ping interval (ms)");  
  Serial.println("t : sets the top to the current value (as long as it is above the current bottom setting)");
  Serial.println("b : sets the bottom to the current value (as long as it is below the current top setting)");
  Serial.println("? : help - you're reading it");
  Serial.println();
  Serial.println(
    "top: " + String(top) 
    + " bottom:" + String(bottom)  
    + " freemode: " + String(freeMode) 
    + " pingInterval: " + String(pingInterval));
}

void stopDesk() {
  digitalWrite(redPin, HIGH);
  digitalWrite(downPin, LOW);
  digitalWrite(upPin, LOW);
  dir = 0;
}

