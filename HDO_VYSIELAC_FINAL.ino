/*    Arduino Long Range Wireless Communication using HC-12
                      Example 01
   by Dejan Nedelkovski, www.HowToMechatronics.com
*/

#include <SoftwareSerial.h>

SoftwareSerial HC12(10, 11); // HC-12 TX Pin, HC-12 RX Pin

byte tarifaPIN = 2;

void setup() {
  pinMode(tarifaPIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  HC12.begin(9600);               // Serial port to HC12

}

void loop() {
 
if(digitalRead(tarifaPIN)==0){
    HC12.print("1");
    digitalWrite(LED_BUILTIN, HIGH);  
  }else{
    HC12.print("0");
    digitalWrite(LED_BUILTIN, LOW);    
  }    

 /*
 HC12.print("1");      // Send that data to HC-12
 delay(1000);
 HC12.print("2");
 delay(1000); 
*/
delay(60000);
 
}
