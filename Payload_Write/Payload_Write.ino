#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <SparkFun_KX13X.h>
#define LAUNCH_BUFFER_SIZE 1600
#define BUFFER_SIZE 1000
#define BUFFER_THRESHOLD 854 


const double convRange64G = .001953;
SparkFun_KX134 a1, a2;
//SparkFun_KX134_SPI a3, a4;
SparkFun_KX134* accels[] = {&a1, &a2};
//SparkFun_KX134_SPI* SPIaccels[] = {&a3};
rawOutputData rawData;
rawOutputData buf[4][BUFFER_SIZE] = {0}, lbuf[4][LAUNCH_BUFFER_SIZE] = {0};
File writeTo, cnt;
int startTime, count, lconfirm;
uint16_t bufLoc[4] = {0}, lbufLoc[4] = {0};
bool launch;


void setup() {
  Wire.begin();
  //SPI.begin();
  Wire.setClock(400000);
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
  // pinMode(23,OUTPUT);
  // digitalWrite(23,HIGH);
  // if(!a3.begin(23)){ //SPI number one
  //   Serial.println("a3 failed!");
  //   while(1);
  // }
  // pinMode(22,OUTPUT);
  // digitalWrite(22,HIGH);
  // if(!a4.begin(22)){ //SPI number two
  //   Serial.println("a4 failed!");
  //   while(1);
  // }
  Serial.println("all accelerometers initilized!");


  for(int i=0;i<2;i++){
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
    a.setOutputDataRate(11); // 1600 Hz
    a.enableTapEngine(true);
    a.enableAccel();
  }
  // for(int i=0;i<1;i++){
  //   SparkFun_KX134_SPI &a = *SPIaccels[i];
  //   a.softwareReset();
  //   delay(5);
  //   a.enableAccel(false);
  //   a.setRange(SFE_KX134_RANGE64G);
  //   a.enableBufferInt();            //  Enables the Buffer interrupt
  //   a.enablePhysInterrupt();        //  Enables interrupt pin 1
  //   a.routeHardwareInterrupt(0x40); //  Routes the data ready bit to pin 1
  //   a.enableSampleBuffer();         // Enable buffer.
  //   a.setBufferOperationMode(0x00); // Enable the buffer to be FIFO.
  //   a.setBufferResolution();
  //   a.setOutputDataRate(11); // 1600 Hz
  //   if(!a.enableSampleBuffer()) Serial.println("enableSampleBuffer failed3");
  //   if(!a.setBufferOperationMode(0x00)) Serial.println("setBufferOperationMode failed3");
  //   if(!a.setOutputDataRate(10)) Serial.println("ODR failed3");
  //   a.enableTapEngine(true);
  //   a.enableAccel();
  // }
  Serial.println("All accelerometers ready!");
  

  //need to add thermometer code (2 Hz)


  pinMode(26,OUTPUT);
  Serial.print("Connecting to SD card...");
  if(!SD.begin(26)){
    Serial.println("cannot connect to SD card!");
    while(1);
  }
  Serial.println("connected to SD card!");


  int c;
  cnt = SD.open("/cnt.txt");
  cnt.read((uint8_t*)&c, sizeof(c));
  c++;
  cnt.close();
  cnt = SD.open("/cnt.txt", FILE_WRITE);
  cnt.seek(0);
  cnt.write((uint8_t*)&c, sizeof(c));
  cnt.flush();
  cnt.close();

  writeTo = SD.open("/raw_accel_data_" + String(c) + ".txt", FILE_WRITE);
  if(!writeTo){
    Serial.printf("Could not open %s!\n", ("/raw_accel_data_" + String(c) + ".txt").c_str());
    while(1);
  }
  Serial.printf("Opened %s!\n", ("/raw_accel_data_" + String(c) + ".txt").c_str());


  lconfirm = 0;
  launch = false;
  count = 0;
  //startTime = millis(); //FOR TESTING
}


void loop() { //add thermometer stuff
  // launch = true; //FOR TESTING

  if(!launch){
    for(uint8_t i=0;i<2/*4*/;i++){
      SparkFun_KX134 &a = *accels[i];
      uint16_t n = a.getSampleLevel();
      if(n >= 498){
        n/=6;
        for(int j=0;j<n;j++){
          a.getRawAccelBufferData(&lbuf[i][lbufLoc[i]],1);
          if(squareMagnitude(lbuf[i][lbufLoc[i]]) >= 100){
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
    // for(uint8_t i=0;i<1;i++){
    //   SparkFun_KX134_SPI &a = *SPIaccels[i];
    //   uint16_t n = a.getSampleLevel();
    //   if(n >= 498){
    //     n/=6;
    //     for(int j=0;j<n;j++){
    //       a.getRawAccelBufferData(&lbuf[i+2][lbufLoc[i+2]],1);
    //       if(squareMagnitude(lbuf[i+2][lbufLoc[i+2]]) >= 100){
    //         lconfirm++;
    //       }
    //       else{
    //         lconfirm = 0;
    //       }
    //       lbufLoc[i+2]++;
    //     }
    //     if(lbufLoc[i+2] >= LAUNCH_BUFFER_SIZE){
    //       lbufLoc[i+2] = 0;
    //     }
    //   }
    // }
    if(lconfirm >= 40){ //threshold is 10 data points in a row
      Serial.println("SHEESH LAUNCHED");
      launch = true;
      startTime = millis();
      for(uint8_t i=0;i<4;i++){
        uint16_t sz = LAUNCH_BUFFER_SIZE;
        uint8_t j = i + 1;
        writeTo.write((uint8_t*)&j,sizeof(j));
        writeTo.write((uint8_t*)&sz,sizeof(sz));
        writeTo.write((uint8_t*)&lbuf[i][lbufLoc[i]],(LAUNCH_BUFFER_SIZE - lbufLoc[i]) * sizeof(lbuf[i][0]));
        writeTo.write((uint8_t*)&lbuf[i][0],lbufLoc[i] * sizeof(lbuf[i][0]));
      }
    }
    return;
  }

  if(millis()-startTime >= 600000){
    Serial.printf("Rocket has (hopefully) landed! %i operations ran in %i ms!", count, millis()-startTime);
    writeTo.flush();
    writeTo.close();
    while(1);
  }
  for(uint8_t i=0;i<2;i++){
    SparkFun_KX134 &a = *accels[i];
    if(a.bufferFull()){
      //Serial.printf("l bozo %i is full\n",i+1);
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
        uint8_t j = i + 1;
        writeTo.write((uint8_t*)&j,sizeof(j));
        writeTo.write((uint8_t*)&bufLoc[i],sizeof(bufLoc[i]));
        writeTo.write((uint8_t*)&buf[i][0],sizeof(buf[i][0]) * bufLoc[i]);
        writeTo.flush();
        bufLoc[i] = 0;
      }
    }
  }
  // for(uint8_t i=0;i<1;i++){
  //   SparkFun_KX134_SPI &a = *SPIaccels[i];
  //   if(a.bufferFull()){
  //     Serial.printf("l bozo %i is full\n",i+3);
  //   }
  //   //Serial.printf("Sensor %d sample level: %d\n", i+2, a.getSampleLevel());
  //   uint16_t n = a.getSampleLevel();
  //   if(n >= 498){
  //     n/=6;
  //     for(int j=0;j<n;j++){
  //       a.getRawAccelBufferData(&buf[i+2][bufLoc[i+2]],1);
  //       count++;
  //       bufLoc[i+2]++;
  //     }
  //     if(bufLoc[i+2]>=BUFFER_THRESHOLD){
  //       uint8_t j = i + 3;
  //       writeTo.write((uint8_t*)&j,sizeof(j));
  //       writeTo.write((uint8_t*)&bufLoc[i+2],sizeof(bufLoc[i+2]));
  //       writeTo.write((uint8_t*)&buf[i+2][0],sizeof(buf[i+2][0]) * bufLoc[i+2]);
  //       writeTo.flush();
  //       bufLoc[i+2] = 0;
  //     }
  //   }
  // }
}

float squareMagnitude(rawOutputData d){
  double x = d.xData * convRange64G, y = d.yData * convRange64G, z = d.zData * convRange64G; 
  return (x * x) + (y * y) + (z * z);
}