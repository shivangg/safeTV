/* record.ino Example sketch for IRLib2
 *  Illustrate how to record a signal and then play it back.
 *  
 *  TSOP pin is 9
 *  IR LED sender pin is 3
 *  (The pin numbers are pin 3 for Uno and use pin 9 for the Leonardo and Mega)
 *  Check pin for Pro-Mini
 *  Assuming it to be 3 
 *  
 *  feedback_switch is at 11
 *  feedback_LED is at 7
 *  
 *  
 */

 

#define feedback_switch A5
#define sensor A1 // Sharp IR 
#define DangerThreshold 325
#define feedback_LED 7
#define TSOP 9
 
#include <IRLibDecodeBase.h>  //We need both the coding and
#include <IRLibSendBase.h>    // sending base classes
#include <IRLib_P01_NEC.h>    //Lowest numbered protocol 1st
#include <IRLib_P02_Sony.h>   // Include only protocols you want
#include <IRLib_P03_RC5.h>
#include <IRLib_P04_RC6.h>
#include <IRLib_P05_Panasonic_Old.h>
#include <IRLib_P07_NECx.h>
#include <IRLib_HashRaw.h>    //We need this for IRsendRaw
#include <IRLibCombo.h>       // After all protocols, include this
// All of the above automatically creates a universal decoder
// class called "IRdecode" and a universal sender class "IRsend"
// containing only the protocols you want.
// Now declare instances of the decoder and the sender.
IRdecode myDecoder;
IRsend mySender;

// Include a receiver either this or IRLibRecvPCI or IRLibRecvLoop
#include <IRLibRecv.h>
IRrecv myReceiver(TSOP); //pin number for the receiver

// Storage for the recorded code
uint8_t codeProtocol;  // The type of code
uint32_t codeValue;    // The data bits if type is not raw
uint8_t codeBits;      // The length of the code in bits

//These flags keep track of whether we received the first code 
//and if we have have received a new different code from a previous one.
bool gotOne, gotNew; 
bool dangerous = false;
bool wasDangerous = false;

void setup() {
  gotOne=false; gotNew=false;
  codeProtocol=UNKNOWN; 
  codeValue=0; 
  //  pinMode(button_pin, INPUT);
  Serial.begin(9600);
  delay(2000);while(!Serial);//delay for Leonardo
  Serial.println(F("Send a code from your remote and we will record it."));
  Serial.println(F("Type any character and press enter. We will send the recorded code."));
  Serial.println(F("Type 'r' special repeat sequence."));
  pinMode(feedback_switch, INPUT);
  pinMode(feedback_LED, OUTPUT);
  myReceiver.enableIRIn(); // Start the receiver
}

// Return True if dangerously close to TV

bool inDanger(){
  float volts = analogRead(sensor);
  float distance = (125 / (volts - 1)) * 1000;
  delay(100); // slow down serial port 

    //  To print the distance from  the Sharp sensor
    //  Serial.println(distance);

  if (distance <= DangerThreshold){
      dangerous = true;
      Serial.println("IN DANGER Zone");
      return true;       // turn on pullup resistors  
  }
  else{
    dangerous = false;
    Serial.println("Safe Zone");
    return false;
  } 
}

float getDistance(){
  float volts = analogRead(sensor);
  float distance = (125 / (volts - 1)) * 1000;
  return distance;
}

// Stores the code for later playback
void storeCode(void) {
  gotNew=true; gotOne=true;
  codeProtocol = myDecoder.protocolNum;
  Serial.print(F("Received "));
  Serial.print(Pnames(codeProtocol));
  if (codeProtocol==UNKNOWN) {
    Serial.println(F(" saving raw data."));
  //    myDecoder.dumpResults();
    
    gotOne=false;
    gotNew=false;
    codeValue = REPEAT_CODE;
  }
  else {
    if (myDecoder.value == REPEAT_CODE) {
      // Don't record a NEC repeat value as that's useless.
      Serial.println(F("repeat; ignoring."));
    } else {
      codeValue = myDecoder.value;
      codeBits = myDecoder.bits;
    }
    Serial.print(F(" Value:0x"));
    Serial.println(codeValue, HEX);
  }
}
void sendCode(void) {
  if( !gotNew ) {//We've already sent this so handle toggle bits
    if (codeProtocol == RC5) {
      codeValue ^= 0x0800;
    }
    else if (codeProtocol == RC6) {
      switch(codeBits) {
        case 20: codeValue ^= 0x10000; break;
        case 24: codeValue ^= 0x100000; break;
        case 28: codeValue ^= 0x1000000; break;
        case 32: codeValue ^= 0x8000; break;
      }      
    }
  }
  gotNew=false;
  if(codeProtocol== UNKNOWN) {
    //The raw time values start in decodeBuffer[1] because
    //the [0] entry is the gap between frames. The address
    //is passed to the raw send routine.
    codeValue=(uint32_t)&(recvGlobal.decodeBuffer[1]);
    //This isn't really number of bits. It's the number of entries
    //in the buffer.
    codeBits=recvGlobal.decodeLength-1;
    Serial.println(F("Sent raw"));
  }
  mySender.send(codeProtocol,codeValue,codeBits);
  if(codeProtocol==UNKNOWN) return;
  Serial.print(F("Sent "));
  Serial.print(Pnames(codeProtocol));
  Serial.print(F(" Value:0x"));
  Serial.println(codeValue, HEX);
}

void loop() {
//  if (Serial.available()) {
    Serial.println(analogRead(feedback_switch));
    if ( analogRead(feedback_switch) < 1000 ){
        digitalWrite(feedback_LED, LOW);
        // wasDangerous = inDanger();
        // delay(2000);
        // Serial.print( "dangerous  ");
        // Serial.print( dangerous);
        // Serial.print( "\t WasDangerous  ");
        // Serial.print( wasDangerous);
        // if ( dangerous & wasDangerous == 0) {
        // if (Serial.available()) {
        //    uint8_t C= Serial.read();
        //    if(C=='r')codeValue=REPEAT_CODE;
        //  if(gotOne) {
        //     Serial.println("Sending new code!");
        //     sendCode();
        //     myReceiver.enableIRIn(); // Re-enable receiver
        //   }
        // }
        if(getDistance()  < DangerThreshold )
        {
          delay(2000);
          sendCode();
          myReceiver.enableIRIn(); // Re-enable receiver
          Serial.println("Closing TV");
          while(getDistance()  < DangerThreshold && analogRead(feedback_switch) < 1000 );
          Serial.println("Opeing TV");
          
          delay(2000);
          sendCode();
          myReceiver.enableIRIn(); // Re-enable receiver
        }
    }
    //    If feedback switch is HIGH, learn  
    else if (myReceiver.getResults()) {
      digitalWrite(feedback_LED, HIGH);
      myDecoder.decode();
      Serial.println("Learning new code!");
      storeCode();
      myReceiver.enableIRIn(); // Re-enable receiver
    }
}

