#include <Arduino.h>

constexpr bool DEBUG = false;

byte digital;
byte video;
byte camera;
byte ninetyfour;

const unsigned int numTouchPins = 9;
const unsigned int touchPins[numTouchPins] = {0,1,16,15,17,18,19,22,23}; 

// calibration
unsigned int minimum = 3000; 
unsigned int maximum = 4500; 

int current[numTouchPins]; 
unsigned int previous[numTouchPins];

// time guard for serial
unsigned long printTime;
const unsigned int printInterval = 100;
unsigned long pastPrintTime = 0;

// smoothing
const unsigned int numReadings = 60;
unsigned int readings[numTouchPins][numReadings];      // the readings
unsigned int readIndex[numTouchPins] = {0};              // the index of the current reading
unsigned int total[numTouchPins] = {0};                  // the running total
unsigned int average[numTouchPins] = {0};                // the average
const unsigned int smoothingInterval = 1;
unsigned long smoothingTime[numTouchPins];
unsigned long pastSmoothingTime[numTouchPins] = {0};

unsigned int ledInterval = 2000;
unsigned long ledTime;
unsigned long pastLedTime = 0;
bool ledLatch = false;

int led = 13;

// midi
int channel = 1;
bool sending = false;
bool did_send[numTouchPins];
elapsedMillis midiSendTimer;
int midiSendMaxTime = 5; // millis

void setup() {
    if (DEBUG) {
        while (!Serial)
        ;
        // dvcam debug baby
    }

    pinMode(led, OUTPUT);

    for (unsigned int i = 0; i < numTouchPins; i++) {
        // initialize all readings to 0
        for (unsigned int y = 0; y < numReadings; y++) {
            readings[i][y] = 0;
        }
    }

    for (int i = 0; i < 30; i++) {
        digitalWrite(led, HIGH);
        delay(50);
        digitalWrite(led, LOW);
        delay(50);
    }

    if (DEBUG) {
        Serial.println("\n\n\twe are running baby");
        delay(1500);
    }
}

void sendMidi(int ccNum, int value, int chan) {
    if(midiSendTimer > midiSendMaxTime) {
        usbMIDI.sendControlChange(ccNum, value, chan); 
        midiSendTimer = 0;
    }
}

void loop() {

    for (unsigned int i = 0; i < numTouchPins; i++) {
        smoothingTime[i] = millis();
    }

    ledTime = millis();

    printTime = millis();


    for (unsigned int i = 0; i < numTouchPins; i++) {
        current[i] = touchRead(touchPins[i]);
        current[i] = map(current[i], minimum, maximum, 0, 127);
        current[i] = constrain(current[i], 0, 127);
    }

    for (unsigned int i = 0; i < numTouchPins; i++) {

        if (smoothingTime[i] - pastSmoothingTime[i] >= smoothingInterval) {

            // subtract the last reading:
            total[i] = total[i] - readings[i][readIndex[i]];
            // read from the touchiiii:
            readings[i][readIndex[i]] = current[i];
            // add the reading to the total:
            total[i] = total[i] + readings[i][readIndex[i]];
            // advance to the next position in the array:
            readIndex[i] = readIndex[i] + 1;

            // if we're at the end of the array...
            if (readIndex[i] >= numReadings) {
                // ...wrap around to the beginning:
                readIndex[i] = 0;
            }

            // calculate the average:
            average[i] = total[i] / numReadings;

            // update time guard
            pastSmoothingTime[i] = smoothingTime[i];

            // send
            if(average[i] != previous[i]) {
                previous[i] = average[i]; 
                sendMidi(i+1, average[i], channel);
                did_send[i] = true;
            }
        }
    }

    for (auto i = 0; i < numTouchPins; i++) {
        // make sure that no touch sends cc 0 value
        if((did_send[i]) && (average[i] < 1)) {
            usbMIDI.sendControlChange(i+1, 0, channel); 
            did_send[i] = false; 
        }
    }

    // check if we are sending midi or not
    for (unsigned int i = 0; i < numTouchPins; i++) {
        if (average[i] > 0) {
            sending = true;
            break;
        }
        sending = false;
    }

    // if we are sending midi change led blinking interval
    if (sending) {
        ledInterval = 25+random(500);
    } else {
        ledInterval = 2000;
    }

    // time to blink
    if (ledTime - pastLedTime >= ledInterval) {
        if (ledLatch == false) {
            ledLatch = true;
            digitalWrite(led, HIGH);
        } else {
            ledLatch = false;
            digitalWrite(led, LOW);
        }
        pastLedTime = ledTime;
    }

    if (DEBUG) {	
        if (printTime - pastPrintTime >= printInterval) {
            Serial.print("\n\n\n");
            for (unsigned int i = 0; i < numTouchPins; i++) {
                Serial.print("\n\t###### THING ");
                Serial.print(i+1);
                Serial.println(" ######");

                Serial.print("\ttouch : ");
                Serial.println(current[i]);
                Serial.print("\tsmooth : ");
                Serial.println(average[i]);

                Serial.print("\tunproc reading: ");
                Serial.print(touchRead(touchPins[i]));
                Serial.print("\n");

                Serial.print("did send?");
                Serial.println(did_send[i]);
            }
            pastPrintTime = printTime;
        }
    }
    /* while (usbMIDI.read()); // read and discard any incoming MIDI messages */
}
