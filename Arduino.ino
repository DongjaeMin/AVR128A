#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN A1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include <SoftwareSerial.h>
SoftwareSerial A_Serial(5,6);

#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <SPI.h>

#define RST_PIN 9
#define SS_PIN 10
#define IRQ_PIN 2

int count=0;
String myStr="";
char msg[100];

int card_state=0;

MFRC522 mfrc(SS_PIN, RST_PIN);

void isr()
{
  card_state=1;
}

 void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin(9600);
  A_Serial.begin(9600);
  SPI.begin();
  mfrc.PCD_Init();
  mfrc.PCD_WriteRegister(MFRC522::ComIrqReg, 0x80); //Clear interrupts
  mfrc.PCD_WriteRegister(MFRC522::ComIEnReg, 0x7F); //Enable all interrupts
  //pinMode(IRQ_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN),isr,RISING);
  pinMode(IRQ_PIN, INPUT);
  digitalWrite(IRQ_PIN, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!mfrc.PICC_IsNewCardPresent())
      return;
  if(!mfrc.PICC_ReadCardSerial())
      return;
      
          
  count++;
  if(card_state==1&&count==1)
  {
    myStr="N";
  }
  if(card_state==1&&count==2)
  {
    myStr="S";
  }
  if(card_state==1&&count==3)
  {
    myStr="D";
    count=0;
  }

  int h = dht.readHumidity();
  int t = dht.readTemperature();

  sprintf(msg,":%d:%d",t,h);
  myStr=myStr+msg;
  Serial.println(myStr);
  A_Serial.println(myStr);
  card_state=0;
  delay(1000);
}
