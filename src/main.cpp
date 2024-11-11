#include <Arduino.h>
#include <EEPROM.h>

#include "PinDefinitionsAndMore.h"

#include <IRremote.hpp>

#if !defined(ARDUINO_ESP32C3_DEV)
#define DISABLE_CODE_FOR_RECEIVER
#endif

#define DELAY_AFTER_SEND 2000
#define DELAY_AFTER_LOOP 5000

#define record_mode_button 4
#define send_command_button 5
#define record_mode_led 6

uint8_t record_mode = 0;

IRrecv IrReceive;

struct RemoteCode {

  uint32_t raw_data[4];

  uint16_t command;
  
  uint16_t address;
  
  uint16_t bits;

  char protocol[20];
};


void setup()
{
    Serial.begin(115200);
    pinMode(record_mode_button, INPUT_PULLUP);
    pinMode(send_command_button, INPUT_PULLUP);
    pinMode(record_mode_led, OUTPUT);
    // pinMode(decode_pin, INPUT_PULLUP);
    while (!Serial)
        ;

    IrReceive.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    Serial.print(F("Ready to receive IR signals of protocols: "));
}

// uint32_t tRawData[] = {0xC008A66, 0x446E6E6E, 0x2900};
// uint32_t tRawOffData[] = {0xC000A66, 0x446E6E6E, 0x2980};

void receive_ir_data()
{
    if (IrReceiver.decode())
    {
        Serial.print(F("Decoded protocol: "));
        Serial.print(getProtocolString(IrReceiver.decodedIRData.protocol));
        Serial.print(F(", decoded raw data: "));
#if (__INT_WIDTH__ < 32)
        Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
#else
        PrintULL::print(&Serial, IrReceiver.decodedIRData.decodedRawData, HEX);
#endif
        Serial.print(F(", decoded address: "));
        Serial.print(IrReceiver.decodedIRData.address, HEX);
        Serial.print(F(", decoded command: "));
        Serial.println(IrReceiver.decodedIRData.command, HEX);

        RemoteCode remote_code ;

         remote_code.address = IrReceiver.decodedIRData.address;
            remote_code.command = IrReceiver.decodedIRData.command;
        
        // Check if decodedIRData.rawDataPtr->rawlen is greater than 4 
        

        // Serial.println("Len of raw data : " + String(IrReceiver.decodedIRData));
        // for (uint_fast8_t i = 0; i < IrReceiver.decodedIRData.rawDataPtr->rawlen; i++)
        // {
        //     remote_code.raw_data[i] = IrReceiver.decodedIRData.rawDataPtr->rawbuf[i];
        //     Serial.print(IrReceiver.decodedIRData.rawDataPtr->rawbuf[i], HEX);
        // }


        String protocol = getProtocolString(IrReceiver.decodedIRData.protocol);

        strcpy(remote_code.protocol,  protocol.c_str());
        
        remote_code.bits = IrReceiver.decodedIRData.numberOfBits;


        EEPROM.put(0, remote_code);

        Serial.println(F("Data saved to EEPROM"));

        record_mode = 0;

        digitalWrite(record_mode_led, LOW);


        
        IrReceiver.resume();
    }
}

void loop()
{
    // if(digitalRead(button_pin) == LOW){
    //     Serial.println(F("Button pressed"));
    //     for(uint8_t i = 0; i < 3; i++){
    //         IrSender.sendSony(0x1, 0x15, 2, 12);
    //         delay(5);
    //     }
    // } else if(digitalRead(ac_button_pin) == LOW){
    //     Serial.println(F("AC Button pressed"));

    //     IrSender.sendPulseDistanceWidthFromArray(38, 1150, 400, 1100, 2400, 1100, 450, &tRawData[0], 79, PROTOCOL_IS_LSB_FIRST, 0,0);
    //     delay(DELAY_AFTER_SEND);

    //     IrSender.sendPulseDistanceWidthFromArray(38, 1150, 400, 1150, 2400, 1150, 450, &tRawOffData[0], 79, PROTOCOL_IS_LSB_FIRST, 0,0);
    //     delay(DELAY_AFTER_SEND);

    // }

    if (digitalRead(record_mode_button) == LOW)
    {
        if(record_mode == 1){
            record_mode = 0;
            digitalWrite(record_mode_led, LOW);
            Serial.println(F("Send mode activated"));
        } else {
            record_mode = 1;
            digitalWrite(record_mode_led, HIGH);
            Serial.println(F("Record mode activated"));
        }
    }
  
  if (digitalRead(send_command_button) == LOW)
    {

        if(record_mode == 1){
            Serial.println(F("Record mode activated"));
            return;
        }
        Serial.println(F("Send command button pressed"));
        // IrSender.sendSony(0x1, 0x15, 2, 12);
        // Decoded protocol: Sony, decoded raw data: 95, decoded address: 1, decoded command: 15
        
        // Get data from eeprom address 0

        RemoteCode remote_code;
        
        EEPROM.get(0, remote_code);

        Serial.println("Command : " + String(remote_code.command));
        Serial.println("Address : " + String(remote_code.address));
        Serial.println("Protocol : " +  String(remote_code.protocol));
        Serial.println("Bits : " +  String(remote_code.bits));

        if(strcmp(remote_code.protocol, "Sony") == 0){
                IrSender.sendSony(remote_code.address, remote_code.command, 2,  remote_code.bits);
        } else if(strcmp(remote_code.protocol, "PulseDistance")==0){
            IrSender.sendPulseDistanceWidthFromArray(38, 1150, 400, 1100, 2400, 1100, 450, &remote_code.raw_data[0], remote_code.bits, PROTOCOL_IS_LSB_FIRST, 0,0);
        } else if(strcmp(remote_code.protocol, "NEC")==0){
            IrSender.sendNEC(remote_code.address, remote_code.command, 0);
        } else if(strcmp(remote_code.protocol, "Samsung") == 0){
            //Working below
            IrSender.sendSamsung(remote_code.address,remote_code.command,  0);

            // For Samsung TV Q7F number of repeats should be 1
            // IrSender.sendSamsung(remote_code.address, remote_code.command, 1);
            Serial.println(F("Samsung command sent"));
        }
        else {
            Serial.println(F("Protocol not supported"));
        }



        delay(5);
    }

    if (record_mode == 1)
    {

        IrReceiver.restartAfterSend(); // Is a NOP if sending does not require a timer.

        // wait for the receiver state machine to detect the end of a protocol
        delay((RECORD_GAP_MICROS / 1000) + 5);
        receive_ir_data();
    }
   

    delay(100);
}
