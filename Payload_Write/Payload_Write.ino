#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <SparkFun_KX13X.h>
#include <vector>


struct AccelInfo {
  SparkFun_KX134* accel;
  const uint8_t num;
};

struct Reading {
  int16_t x,y,z;
};

const double convRange64G = .001953;
SparkFun_KX134 a1, a2, a3, a4;
AccelInfo accels[] = {{&a1,1},{&a2,2}};//,{&a3,3},{&a4,4}};
rawOutputData rawData;
File writeTo, a1Data, a2Data, a3Data, a4Data;
std::vector<Reading> buffer;
int count; 
int startTime;
bool launch;

void setup() {
  Wire.begin();
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

  for(auto a:accels){
    a.accel->softwareReset();
    delay(5);
    a.accel->enableAccel(false);
    a.accel->setRange(SFE_KX134_RANGE64G);
    a.accel->enableBufferInt();            //  Enables the Buffer interrupt
    a.accel->enablePhysInterrupt();        //  Enables interrupt pin 1
    a.accel->routeHardwareInterrupt(0x40); //  Routes the data ready bit to pin 1
    a.accel->enableSampleBuffer();         // Enable buffer.
    a.accel->setBufferOperationMode(0x00); // Enable the buffer to be FIFO.
    a.accel->setBufferResolution();
    a.accel->setOutputDataRate(9); //200hz 400 for testing
    a.accel->enableTapEngine(true);
    a.accel->enableAccel();
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

  // if(SD.exists("/accel1_data.csv")){
  //   SD.remove("/accel1_data.csv");  
  //   Serial.println("erasing accel1_data.csv!");
  // }  
  // if(SD.exists("/accel2_data.csv")){
  //   SD.remove("/accel2_data.csv");  
  //   Serial.println("erasing accel2_data.csv!");
  // }  
  // if(SD.exists("/accel3_data.csv")){
  //   SD.remove("/accel3_data.csv");  
  //   Serial.println("erasing accel3_data.csv!");
  // }  
  // if(SD.exists("/accel4_data.csv")){
  //   SD.remove("/accel4_data.csv");
  //   Serial.println("erasing accel4_data.csv!");
  // }
  if(SD.exists("/raw_accel_data.txt")){ //testing
    SD.remove("/raw_accel_data.txt");
    Serial.println("erasing raw_accel_data.txt!");
  }

  writeTo = SD.open("/raw_accel_data.txt",FILE_WRITE);
  if(!writeTo){
    Serial.println("Could not open raw_accel_data.txt!");
    while(1);
  }
  Serial.println("Opened raw_accel_data.txt!");


  count = 0;
  launch = false;
  startTime = millis(); //will be moved to launch detection
}


void loop() { //add thermometer stuff
  if(millis()-startTime >= 1000/*600000*/){
    Serial.printf("%i operations ran in %i ms!!!\n",count, millis());
    // writeTo.flush();
    // writeTo.close();

    // int8_t accelNum;
    // uint16_t numToRead;
    // writeTo = SD.open("/raw_accel_data.txt");
    // size_t pos = writeTo.position();

    // while(count){
    //   writeTo.seek(pos);
    //   writeTo.read((uint8_t*)&accelNum,sizeof(accelNum));
    //   writeTo.read((uint8_t*)&numToRead,sizeof(numToRead));
    //   std::vector<Reading> readings(numToRead);
    //   count-=(int)numToRead;

    //   for(int i=0;i<(int)numToRead;i++){
    //     writeTo.read((uint8_t*)&readings[i].x, 2);
    //     writeTo.read((uint8_t*)&readings[i].y, 2);
    //     writeTo.read((uint8_t*)&readings[i].z, 2);
    //   }
    //   pos = writeTo.position();
    //   writeTo.close();

    //   if(accelNum == 1){
    //     Serial.println("a1 has data!");
    //     writeTo = SD.open("/accel1_data.csv",FILE_WRITE);
    //   }
    //   else if(accelNum == 2){
    //     Serial.println("a2 has data!");
    //     writeTo = SD.open("/accel2_data.csv",FILE_WRITE);
    //   }
    //   else if(accelNum == 3){
    //     Serial.println("a3 has data!");
    //     writeTo = SD.open("/accel3_data.csv",FILE_WRITE);
    //   }
    //   else{
    //     Serial.println("a4 has data!");
    //     writeTo = SD.open("/accel4_data.csv",FILE_WRITE);
    //   }
    //   for(int i=0;i<(int)numToRead;i++){
    //     writeTo.print(readings[i].x*convRange64G);
    //     writeTo.print(",");
    //     writeTo.print(readings[i].y*convRange64G);
    //     writeTo.print(",");
    //     writeTo.print(readings[i].z*convRange64G);
    //     writeTo.print(",");
    //     writeTo.println();
    //   }
    //   writeTo.flush();
    //   writeTo.close();

    //   writeTo = SD.open("/raw_accel_data.txt");
    //   Serial.printf("%i left!\n",count); 
    // }
    // Serial.println("Done transferring data!");
    while(1);
  }

  for(auto a:accels){
    uint16_t n = a.accel->getSampleLevel();
    if(n >= 502){
      n/=6;
      buffer.resize(n);

      launch = true; //FOR TESTING
      
      if(launch){
        writeTo.write((uint8_t*)&a.num,sizeof(a.num));
        writeTo.write((uint8_t*)&n,sizeof(n));
      }

      for(int i=0;i<(int)n;i++){
        a.accel->getRawAccelBufferData(&rawData,1);

        if(!launch){
          buffer[i].x = rawData.xData;
          buffer[i].y = rawData.yData;
          buffer[i].z = rawData.zData;
          if((buffer[i].x+buffer[i].y+buffer[i].z)*convRange64G > 10){
            startTime = millis();
            launch = true;
            writeTo.write((uint8_t*)&a.num,sizeof(a.num));
            writeTo.write((uint8_t*)&n,sizeof(n));
            for(int j=0;j<i;j++){
              writeTo.write((uint8_t*)&buffer[i].x,sizeof(buffer[i].x)); 
              writeTo.write((uint8_t*)&buffer[i].y,sizeof(buffer[i].y));
              writeTo.write((uint8_t*)&buffer[i].z,sizeof(buffer[i].z));
              count++;
            }
          }
        }
        else{
          writeTo.write((uint8_t*)&rawData.xData,sizeof(rawData.xData)); 
          writeTo.write((uint8_t*)&rawData.yData,sizeof(rawData.yData));
          writeTo.write((uint8_t*)&rawData.zData,sizeof(rawData.zData));
          count++;
        }
      }
      writeTo.flush();
    }
  }
}
