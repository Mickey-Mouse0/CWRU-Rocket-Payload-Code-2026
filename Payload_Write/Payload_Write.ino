#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <SparkFun_KX13X.h>
#include <cmath>
#define LAUNCH_BUFFER_SIZE 1600
#define BUFFER_SIZE 1000
#define BUFFER_THRESHOLD 854 


const double convRange64G = .001953;
SparkFun_KX134 a1, a2, a3, a4;
SparkFun_KX134* accels[] = {&a1,&a2};//,&a3,&a4};
rawOutputData rawData;
rawOutputData buf[4][BUFFER_SIZE], lbuf[4][LAUNCH_BUFFER_SIZE];
File writeTo;
int startTime, count, lconfirm;
uint16_t bufLoc[4] = {0}, lbufLoc[4] = {0};
bool launch;


void setup() {
  Wire.begin();
  Wire.setClock(1000000);
  Serial.begin(115200);
  while(!Serial){
    delay(50);
  }
  Serial.println();
  Serial.println("Serial moniter opened!");


  Serial.print("Connecting to accelerometers...");
  if(!a1.begin(0x1E)){ //I2C number one
    Serial.println("a1 failed!");
    while(1);
  }
  if(!a2.begin(0x1F)){ //I2C number two
    Serial.println("a2 failed!");
    while(1);
  }
  // if(!a3.begin()){
  //   Serial.println("a3 failed!");
  //   while(1);
  // }
  // if(!a4.begin()){
  //   Serial.println("a4 failed!");
  //   while(1);
  // }
  Serial.println("all accelerometers initilized!");

  for(int i=0;i<2/*4*/;i++){
    SparkFun_KX134 &a = *accels[i];
    a.softwareReset();
    delay(5);
    a.enableAccel(false);
    a.setRange(SFE_KX134_RANGE64G);
    a.enableBufferInt();            //  Enables the Buffer interrupt
    a.enablePhysInterrupt();        //  Enables interrupt pin 1
    a.routeHardwareInterrupt(0x40); //  Routes the data ready bit to pin 1
    a.enableSampleBuffer();         // Enable buffer.
    a.setBufferOperationMode(0x00); // Enable the buffer to be FIFO.
    a.setBufferResolution();
    a.setOutputDataRate(11); //1600 Hz
    a.enableTapEngine(true);
    a.enableAccel();
  }
  Serial.println("All accelerometers ready!");
  

  //need to add thermometer code (2 Hz)


  pinMode(26,OUTPUT);
  Serial.print("Connecting to SD card...");
  if(!SD.begin(26)){
    Serial.println("cannot connect to SD card!");
    while(1);
  }
  Serial.println("connected to SD card!");

  if(SD.exists("/raw_accel_data.txt")){ //REMOVEEEEEE
    SD.remove("/raw_accel_data.txt");
    Serial.println("erasing raw_accel_data.txt!");
  }

  // File root = SD.open("/");
  // File entry;
  // while(entry = root.openNextFile()){
  //   entry.name() //asdijpfgasrthpawrhtawerawer
  // }

  // writeTo = SD.open("/raw_accel_data.txt",FILE_WRITE); //CHANGE
  // if(!writeTo){
  //   Serial.println("Could not open raw_accel_data.txt!");
  //   while(1);
  // }
  // Serial.println("Opened raw_accel_data.txt!");

  lconfirm = 0;
  launch = false;
  count = 0;
  startTime = millis(); //FOR TESTING
}


void loop() { //add thermometer stuff
  launch = true; //FOR TESTING

  if(!launch){
    for(uint8_t i=0;i<2/*4*/;i++){
      SparkFun_KX134 &a = *accels[i];
      uint16_t n = a.getSampleLevel();
      if(n >= 498){
        n/=6;
        for(int j=0;j<n;j++){
          a.getRawAccelBufferData(&lbuf[i][lbufLoc[i]],1);
          if(magnitude(lbuf[i][lbufLoc[i]]) >= 10){
            lconfirm++;
          }
          else{
            lconfirm = 0;
          }
          lbufLoc[i]++;
        }
        if(lbufLoc[i] >= LAUNCH_BUFFER_SIZE){
          lbufLoc[i] = 0;
        }
      }
    }
    if(lconfirm >= 40){
      launch = true;
      startTime = millis();
      for(uint8_t i=0;i<4;i++){
        uint16_t sz = LAUNCH_BUFFER_SIZE;
        writeTo.write((uint8_t*)&i,sizeof(i));
        writeTo.write((uint8_t*)&sz,sizeof(sz));
        writeTo.write((uint8_t*)&lbuf[i][lbufLoc[i]],(LAUNCH_BUFFER_SIZE - lbufLoc[i]) * sizeof(buf[i][0]));
        writeTo.write((uint8_t*)&lbuf[i][0],lbufLoc[i] * sizeof(buf[i][0]));
      }
    }
    return;
  }

  if(millis()-startTime >= 6000/*00*/){
    Serial.printf("Rocket has (hopefully) landed! %i operations ran in %i ms!", count, millis()-startTime);
    while(1);
  }
  for(uint8_t i=0;i<2/*4*/;i++){
    SparkFun_KX134 &a = *accels[i];
    if(a.bufferFull()){
      Serial.printf("l bozo %i is full\n",i+1);
    }
    uint16_t n = a.getSampleLevel();
    if(n >= 498){
      n/=6;
      for(int j=0;j<n;j++){
        a.getRawAccelBufferData(&buf[i][bufLoc[i]],1);
        count++;
        bufLoc[i]++;
      }
      if(bufLoc[i]>=BUFFER_THRESHOLD){
        writeTo.write((uint8_t*)&i,sizeof(i));
        writeTo.write((uint8_t*)&bufLoc[i],sizeof(bufLoc[i]));
        writeTo.write((uint8_t*)&buf[i],sizeof(buf[i]));
        writeTo.flush();
        bufLoc[i] = 0;
      }
    }
  }
}

float magnitude(rawOutputData d){
  return sqrt(pow(d.xData,2) + pow(d.yData,2) + pow(d.zData,2));
}