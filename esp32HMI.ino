#define baudrate 9600
#define DE_RE_PIN 4
#define RX 16
#define TX 17
#define SLAVE_ID 0x01

uint8_t arr[256];
uint8_t count = 0;
unsigned long last_recp = 0;

bool CRCcheck(uint8_t arr[],uint8_t leng)
{
  // here itself comes the crc calculation and store in calcuated variable
  if(arr[leng-1] == (calculated & 0XFF) && (arr[leng-2] == (calculated>>8)& 0xFF))
  {
    return true;
  }
  else
  return false;

}

void processFrame(uint8_t arr1[],uint8_t leng)
{
  if(arr1[0]==SLAVE_ID && CRCcheck(arr1,leng))
  {
   

  }

}

void setup() 
{
  pinMode(DE_RE_PIN,OUTPUT);
  digitalWrite(DE_RE_PIN,LOW);
  Serial2.begin(baudrate,SERIAL_8N1,RX,TX);
}

void loop() 
{
  while(Serial2.available())
  {
    arr[count] = Serial2.read();
    last_recp = millis();
    count++;
    if(count>=256)
    {
      break;
    }
  }
   if(millis()-last_recp>=4 && count!=0)
    {
      processFrame(arr,count);
      for(int i = 0;i<count;i++)
      {
        arr[i]=0;
      }
      count = 0;
    
    }

}
