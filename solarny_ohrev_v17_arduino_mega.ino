//20.7.2020
//PRIDAL SOM: PRI DOPUSTANI VODY JE ZAPNUTA FILTRACIA

#include <EEPROM.h>
#include <avr/wdt.h>

/////CLOCK MODULE///////////////////
#include <DS1302.h> 
const int kCePin   = 5;  //RST 
const int kIoPin   = 6;  //DAT
const int kSclkPin = 7;  //CLK
DS1302 rtc(kCePin, kIoPin, kSclkPin);  // Create a DS1302 object.
 
//TLACIDLA
byte buttonVYP = 38, buttonZAP = 40, buttonAUTO = 42, buttonSERVIS = 44;
byte ledVYP = 52, ledZAP = 50, ledAUTO = 48, ledSERVIS = 46;

//RELE
byte rele_filtracia = 49;
byte rele_ohrev = 53;
byte rele_dopustanie = 51;

//DALLAS
#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 33       //PIN TEPLOTA
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

//HC12
#include <SoftwareSerial.h>
SoftwareSerial HC12(10,0); // HC-12 TX Pin, HC-12 RX Pin 
byte incomingByte;
String readBuffer = "";

//HC05 
SoftwareSerial HC05(11,12); // HC-12 TX Pin, HC-12 RX Pin
char r_char;
int r_int;

//PINY
byte tlakPIN = A15;
byte uvPIN = A1;
byte hladinaPIN = 29;

//PREMENNÉ
byte uvAktualne = 0, uvSpinacie = 0;
double teplota = 0; byte pozTeplota = 0;
int tlak = 0, tlakMax = 0, tlakMin = 0;
int tarifa = 0;   
byte pozCasFiltracie = 0; 
byte hladinaHIGH = 0;

bool vymazatchyby = false;

//test trash

//CHYBY
String porucha1= "nic";
String porucha2= "nic";
String porucha3= "nic";
bool PORUCHA = false;

bool PORUCHAMALYTLAK = false;
bool PORUCHANAPUSTANIE = false;

//CASOVACE
unsigned long previousMillis = 0;  
unsigned long previousMillis2 = 0;
const long intervalminuta = 60000;
const long intervalsekunda = 1000;
int casFiltracie = 0;
int casPoruchaTlak = 0;
int casOhrevVody = 0;
int casPripojenyBT = 0;
int casPoruchaHdo = 0;
int casPoruchaCasovac = 0;
int casKontrolaTeploty = 16;
int casNapustaniaVody = 0;

//OSTATNE
byte rezim = 0;
byte rezimDopustania = 1;
byte rezimBT = 0;
bool dopustanievody = false;
bool ohrevvody = false;
bool vysokateplota = false;


bool blik = false;

void setup() {
  pinMode(hladinaPIN, INPUT_PULLUP);
  
  pinMode(ledZAP, OUTPUT);
  pinMode(ledVYP, OUTPUT);
  pinMode(ledAUTO, OUTPUT);
  pinMode(ledSERVIS, OUTPUT);
  
  digitalWrite(ledZAP, LOW);
  digitalWrite(ledVYP, LOW);
  digitalWrite(ledAUTO, LOW);
  digitalWrite(ledSERVIS, LOW);

  pinMode(buttonZAP, INPUT_PULLUP);
  pinMode(buttonVYP, INPUT_PULLUP);
  pinMode(buttonAUTO, INPUT_PULLUP);
  pinMode(buttonSERVIS, INPUT_PULLUP);

  pinMode(rele_filtracia, OUTPUT);
  pinMode(rele_ohrev, OUTPUT);
  pinMode(rele_dopustanie, OUTPUT);

//  digitalWrite(rele_filtracia, LOW);   //OVLÁDANIE RELÉ JE OPAČNÉ HIGH → VYPNUTE RELE
//  digitalWrite(rele_dopustanie, LOW);
//  digitalWrite(rele_ohrev, LOW);
//  delay(2000);
  
  nacitajEEPROM();
  Serial.begin(9600);  
}

void loop() {
wdt_enable(WDTO_8S); //WATCHDOG
  
Time t = rtc.time();

if(t.hr==20 && t.min== 00 && t.sec ==0){
    wdt_disable();
    
    digitalWrite(rele_filtracia, LOW);   
    digitalWrite(rele_dopustanie, LOW);
    digitalWrite(rele_ohrev, LOW);
    delay(9000);

    digitalWrite(rele_ohrev, HIGH);
    delay(9000);
    digitalWrite(rele_dopustanie, HIGH);
    delay(9000);
    digitalWrite(rele_dopustanie, LOW);
    delay(9000);
    digitalWrite(rele_ohrev, LOW);
    delay(9000);
    wdt_enable(WDTO_8S);
}
  
sensors.requestTemperatures(); //načítaj dallas
teplota = sensors.getTempCByIndex(0);
tlak = analogRead(tlakPIN);
tlak = map(tlak, 0, 1023, 0, 100);
if(digitalRead(hladinaPIN)==0){hladinaHIGH = 1;}else{hladinaHIGH = 0;}
//UV sensor
uvAktualne = zistiUVindex(analogRead(uvPIN));

casovace();
poruchy();
rezimbluetooth();

if(porucha1 == "nic" && porucha2 == "nic" && porucha3 == "nic"){PORUCHA = false;}else{PORUCHA = true;}

//    TLACIDLA  A  LEDKY
if(digitalRead(buttonVYP)==0){rezim = 0; EEPROM.write(7, rezim);}
if(digitalRead(buttonZAP)==0){rezim = 1; PORUCHAMALYTLAK=false; EEPROM.write(7, rezim);}
if(digitalRead(buttonAUTO)==0){rezim = 2; EEPROM.write(7, rezim);}  

if(t.hr==16 && t.min==0 && t.sec ==0)PORUCHAMALYTLAK = false;

if(t.hr==8 && t.min==0 && t.sec ==0){
  if(pozCasFiltracie <= 8 && casFiltracie < pozCasFiltracie*60 && casFiltracie != 0)chyba("Nesplneny cas filtracie");
  casFiltracie = 0;
  casOhrevVody = 0;   
  casPripojenyBT = 0;
  casKontrolaTeploty = 0;
  casNapustaniaVody = 0;
}
if(t.min==0 && t.sec ==0)EEPROM.write(8, casFiltracie);

algoritmus();
ovladanieled();


/*
Serial.print(t.hr);
Serial.print(":"); 
Serial.print(t.min);
Serial.print(":"); 
Serial.println(t.sec);

Serial.print("TMP: "); 
Serial.println(tmp);

Serial.print(hodiny);
Serial.print(":");
Serial.println(minuty);

Serial.print("uvAktualne: "); 
Serial.println(uvAktualne);

Serial.print("teplota: "); 
Serial.println(teplota);

Serial.print("tlak: "); 
Serial.println(tlak);

Serial.print("hladinaHIGH: "); 
Serial.println(hladinaHIGH);

Serial.print("CAS FILTRACIE: "); 
Serial.println(casFiltracie);

Serial.print("poz cas *60: "); 
Serial.println(pozCasFiltracie*60);

Serial.println(" ");
*/
wdt_reset();          // uncomment to avoid reboot
     
delay(300);  
}

void algoritmus(){
  Time t = rtc.time();
  
  if((rezimDopustania == 3 && hladinaHIGH == 0 && tarifa == 1 && t.hr>=12 && t.hr<16) || rezimDopustania == 2 && hladinaHIGH == 0){
    dopustanievody = true;
  }else{      
    dopustanievody = false;
  }    
  
  if(uvAktualne > uvSpinacie){ //treba aj teplotu zakomponovat
    ohrevvody=true;
    casOhrevVody = 0;
  }else{
    if(casOhrevVody > 10)ohrevvody=false;    
  }

  if(teplota >= pozTeplota+1)vysokateplota = true;
  if(teplota <= pozTeplota)vysokateplota = false;
  
  if(dopustanievody == true && PORUCHANAPUSTANIE == false){
      digitalWrite(rele_filtracia, HIGH);   
      digitalWrite(rele_dopustanie, HIGH);
      digitalWrite(rele_ohrev, HIGH);
  }else if(PORUCHAMALYTLAK == true || rezim == 0){
      digitalWrite(rele_filtracia, LOW);  
      digitalWrite(rele_dopustanie, LOW);
      digitalWrite(rele_ohrev, LOW);  
  }else if(rezim == 1){
      digitalWrite(rele_filtracia, HIGH);  
      digitalWrite(rele_dopustanie, LOW);
      digitalWrite(rele_ohrev, LOW);      
  }else if(rezim == 2 && ohrevvody == true && vysokateplota == false){
      digitalWrite(rele_filtracia, HIGH);  
      digitalWrite(rele_dopustanie, LOW);
      digitalWrite(rele_ohrev, HIGH);
  }else if((rezim == 2 && tarifa == 1 && casFiltracie <= pozCasFiltracie*60) || casKontrolaTeploty <= 15){
      digitalWrite(rele_filtracia, HIGH);  
      digitalWrite(rele_dopustanie, LOW);
      digitalWrite(rele_ohrev, LOW);
  }else{
      digitalWrite(rele_filtracia, LOW);  
      digitalWrite(rele_dopustanie, LOW);
      digitalWrite(rele_ohrev, LOW);    
  }   
}

void ovladanieled(){

if(rezimBT == 1 || (PORUCHA && blik) ){
    digitalWrite(ledSERVIS, HIGH);
  }else{
    digitalWrite(ledSERVIS, LOW);
  }

if(hladinaHIGH == 0 && rezim != 0){
  if(blik){digitalWrite(ledAUTO, HIGH);}else{digitalWrite(ledAUTO, LOW);}
  }else if(rezim == 2){
    digitalWrite(ledAUTO, HIGH);
  }else{
    digitalWrite(ledAUTO, LOW);
  }

if(rezim == 1){
  digitalWrite(ledZAP, HIGH);
  }else{
  digitalWrite(ledZAP, LOW);
  }

if(PORUCHAMALYTLAK && rezim != 0){  
  digitalWrite(ledVYP, HIGH);
  }else if(tlak > tlakMax && blik && rezim != 0){
  digitalWrite(ledVYP, HIGH);
  }else{
  digitalWrite(ledVYP, LOW);
  }
}

void chyba(String chyba){
Time t = rtc.time();
String datum = String(t.date) + "." + String(t.mon);
String cas = String(t.hr) + ":" + String(t.min);

if(porucha1 == "nic"){porucha1 = datum + " " + cas + " - " + chyba;
  }else if(porucha2 == "nic"){porucha2 = datum + " " + cas + " - " + chyba;
    }else if(porucha3 == "nic"){porucha3 = datum + " " + cas + " - " + chyba;
      }else{porucha1 = porucha2;
            porucha2 = porucha3;
            porucha3 = datum + " " + cas + " - " + chyba;
      }  
}

void rezimbluetooth(){
if(digitalRead(buttonSERVIS)==0){
   if(rezimBT == 0){
      rezimBT = 1;
      casPripojenyBT = 0;
    }else{
      rezimBT = 0;
    }
}
if(casPripojenyBT>6)rezimBT = 0;

if(rezimBT == 0){
  HC05.end();
  HC12.begin(9600);
  bluetoothHDO();
}

if(rezimBT == 1){
  HC12.end();
  HC05.begin(9600);
  bluetoothMobile();
}
}


void casovace(){  
    unsigned long currentMillis = millis();
 
    if (currentMillis - previousMillis >= intervalminuta) {
    previousMillis = currentMillis;    
    if(digitalRead(rele_filtracia)==1)casFiltracie++;
    if(digitalRead(rele_dopustanie)==1 && rezimDopustania == 3)casNapustaniaVody++;
    casOhrevVody++;
    casPripojenyBT++;
    casPoruchaHdo++;
    casPoruchaCasovac++;
    casKontrolaTeploty++;
    }

    if (currentMillis - previousMillis2 >= intervalsekunda) {
    previousMillis2 = currentMillis;

    if(blik == false){
      blik = true;
    }else{
      blik = false;
    }

          
    if(tlak < tlakMin && digitalRead(rele_filtracia)==1){
      casPoruchaTlak++;
    }else{
      casPoruchaTlak = 0;
    }        
       
    }
}

void poruchy(){
if(casPoruchaTlak >= 60){
      PORUCHAMALYTLAK = true;
      casPoruchaTlak = 0;
}  

if(casPoruchaHdo >= 20){
  chyba("Problem HDO signal");
  casPoruchaHdo = 0;
}

Time t = rtc.time();
if(t.min != 0)casPoruchaCasovac = 0;
if(casPoruchaCasovac > 3){
  chyba("Nie je nastaveny cas");
  casPoruchaCasovac = 0;
  }
if(t.hr>24 || t.min > 60 || t.min < 0 || t.hr < 0)chyba("Casovac porucha");

if(casNapustaniaVody >= 60){
  chyba("Napustanie vody > 1h");
  casNapustaniaVody = 0;
  PORUCHANAPUSTANIE = true;
  }
}

void setTime(byte timeMin, byte timeHod, byte casDen, byte casMesiac){
  rtc.writeProtect(false);            //zistit ci nemusim zapnut po nastaveni
  rtc.halt(false);
  Time t(2017, casMesiac, casDen, timeHod, timeMin, 30, Time::kThursday); //Change this line to set time  ex 2015 26/2 9:09:50    -   Time t(2015, 2, 26, 20, 47, 35, Time::kThursday);
  rtc.time(t);
  rtc.writeProtect(true); //??
}

void bluetoothHDO(){ 
 while (HC12.available()) {             // If HC-12 has data
    incomingByte = HC12.read();          // Store each icoming byte from HC-12
    readBuffer += char(incomingByte);    // Add each byte to ReadBuffer string variable
    tarifa = readBuffer.toInt();
      //v tarifa máme načítanú hodnotu z druhého arduina
    readBuffer = "";
    casPoruchaHdo = 0;
 }  
}

void bluetoothMobile(){  
  
  byte casHod = 0, casMin = 0, casDen = 0, casMesiac = 0;
  Time t = rtc.time(); 
  
  while(HC05.available() > 0){
           r_char = HC05.read();      // Reads a character
           delay(200);
                      
           if(r_char == 'q')pozTeplota = HC05.read(); 
           if(r_char == 'a')casDen = HC05.read();
           if(r_char == 's')casMesiac = HC05.read();          
           if(r_char == 'w')casHod = HC05.read();     
           if(r_char == 'e'){casMin = HC05.read();
                            setTime(casMin,casHod,casDen,casMesiac);}     
           if(r_char == 'r')uvSpinacie = HC05.read(); 
           if(r_char == 't')pozCasFiltracie = HC05.read();  
           if(r_char == 'z')vymazatchyby = HC05.read();  
           if(r_char == 'u')rezimDopustania = HC05.read();        
           if(r_char == 'i')tlakMax = HC05.read();   
           if(r_char == 'o'){tlakMin = HC05.read(); ulozEEPROM();}
             
           if(r_char == 'm'){

             String data;
             String d1=String(pozTeplota);
             String d2=String(t.hr);
             String d3=String(t.min);
             String d4=String(teplota);
             String d5=String(uvSpinacie);
             String d6=String(uvAktualne);    
             String d7=String(pozCasFiltracie);  
             String d8=String(tlak);
             String d9=String(tlakMax); 
             String d10=String(tlakMin);
             String d11=String(hladinaHIGH);
             String d12=String(tarifa);
             String d13=String(rezimDopustania);
                                 
             data = d1+','+d2+','+d3+','+d4+','+d5+','+d6+','+d7+','+d8+','+d9+','+d10+','+d11+','+d12+','+d13;

             data = data + ',' + porucha1 + ',' + porucha2 + ',' + porucha3;
              
             HC05.print(data);
             Serial.println(data);
           }          
           delay(100);          
      }
      if(vymazatchyby==1){porucha1 = "nic";porucha2 = "nic";porucha3 = "nic";vymazatchyby=0;PORUCHANAPUSTANIE = false;}
}

void ulozEEPROM(){
  EEPROM.write(1, pozTeplota);
  EEPROM.write(2, uvSpinacie);
  EEPROM.write(3, pozCasFiltracie);
  EEPROM.write(4, rezimDopustania);
  EEPROM.write(5, tlakMax);
  EEPROM.write(6, tlakMin);  
}

void nacitajEEPROM(){  
  pozTeplota = EEPROM.read(1);
  uvSpinacie = EEPROM.read(2);
  pozCasFiltracie = EEPROM.read(3);
  rezimDopustania = EEPROM.read(4);
  tlakMax = EEPROM.read(5);
  tlakMin = EEPROM.read(6);
  rezim = EEPROM.read(7);
  casFiltracie = EEPROM.read(8);
}

byte zistiUVindex(int sensorValue){
  
  int voltage = (sensorValue * (5.0 / 1023.0))*1000;
  byte UVIndex= 0;
  
  if(voltage<50)
  {
    UVIndex = 0;
  }else if (voltage>50 && voltage<=227)
  {
    UVIndex = 0;
  }else if (voltage>227 && voltage<=318)
  {
    UVIndex = 1;
  }
  else if (voltage>318 && voltage<=408)
  {
    UVIndex = 2;
  }else if (voltage>408 && voltage<=503)
  {
    UVIndex = 3;
  }
  else if (voltage>503 && voltage<=606)
  {
    UVIndex = 4;
  }else if (voltage>606 && voltage<=696)
  {
    UVIndex = 5;
  }else if (voltage>696 && voltage<=795)
  {
    UVIndex = 6;
  }else if (voltage>795 && voltage<=881)
  {
    UVIndex = 7;
  }
  else if (voltage>881 && voltage<=976)
  {
    UVIndex = 8;
  }
  else if (voltage>976 && voltage<=1079)
  {
    UVIndex = 9;
  }
  else if (voltage>1079 && voltage<=1170)
  {
    UVIndex = 10;
  }else if (voltage>1170)
  {
    UVIndex = 11;
  }
  
  return UVIndex;
  }
