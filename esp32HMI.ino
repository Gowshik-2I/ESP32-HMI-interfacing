#define baudrate 9600
#define DE_RE_PIN 4
#define RX 16
#define TX 17
#define SLAVE_ID 0x01

uint8_t arr[256];
uint8_t count = 0;
unsigned long last_recp = 0;

uint16_t CRC_Calc(uint8_t arr[],uint8_t leng)
{
    //CRC calculation
  uint16_t calculated = 0xFFFF;
  for(int i = 0;i<leng-2;i++)
  {
    calculated = arr[i] ^ calculated ;
    for(int j = 0 ;j<8;j++)
    {
      if(calculated & 0x0001)
      {
        calculated = calculated >> 1;
        calculated = calculated ^ 0xA001;
      }
      else
      {
        calculated = calculated >> 1;
      }
    }
  }
  return calculated;
}

bool CRCcheck(uint8_t arr[],uint8_t leng)
{
// CRC checking
  uint16_t crc = CRC_Calc(arr,leng);
  if(arr[leng-2] == ( crc & 0XFF) && (arr[leng-1] == (crc >>8) & 0xFF))
  {
    return true;
  }
  else
  return false;
}

void response(uint8_t arr1[],uint8_t leng)
{
  int sensor_reading = 0x1234; //sending a constant
  uint8_t frame[256];
  frame[0] = SLAVE_ID;
  frame[1] = 0x03;
  frame[2] = 0x02;
  frame[3] = (sensor_reading >>8)& 0xFF;
  frame[4] = sensor_reading & 0xFF;
  uint16_t crc = CRC_Calc(frame,7);
  frame[5] = (crc) & 0xFF;
  frame[6] = (crc>>8) & 0xFF;
  digitalWrite(DE_RE_PIN,HIGH);
  Serial2.write(frame,7);
  Serial2.flush();
  digitalWrite(DE_RE_PIN,LOW);
  //TX debug
    Serial.print("TX : ");
    for(int i = 0 ;i<7;i++)
    {
    Serial.print(frame[i]);
    Serial.print(' ');
    }
   Serial.println();
    //TX debug
}

void processFrame(uint8_t arr1[],uint8_t leng)
{
  if(arr1[0]==SLAVE_ID && CRCcheck(arr1,leng))
  {
   if(arr1[1] == 0x03)
   {
    Serial.println("inside process frame");
    uint8_t register_add_high = arr1[2];
    uint8_t register_add_low = arr1[3];
    uint8_t register_count_high = arr1[4];
    uint8_t register_count_low = arr1[5];
    //RX debug
    Serial.print("RX : ");
    for(int i = 0 ;i<leng;i++)
    {
    Serial.print(arr[i]);
    Serial.print(' ');
    }
   Serial.println();
    //RX debug
   response(arr1,leng);
   }
  }
}

void setup() 
{
  pinMode(DE_RE_PIN,OUTPUT);
  digitalWrite(DE_RE_PIN,LOW);
  Serial2.begin(baudrate,SERIAL_8N1,RX,TX);
  Serial.begin(9600);
  Serial.println("esp is ready");
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
  if(millis()-last_recp >= 6 && count!=0 )
    {
      Serial.println("inside if");
      Serial.print("count:");
      Serial.println(count);
      processFrame(arr,count);
      count = 0; 
    }
}
