#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <vector>

struct dataLoc{
  unsigned long loc;
  uint16_t num; 
};


const double convRange64G = .001953;
std::vector<dataLoc> a1, a2, a3, a4;
File toRead, cnt;


void setup(){
  Wire.begin();  
  Serial.begin(115200);
  Wire.setClock(1000000);
  while(!Serial){
    delay(50);
  }


  pinMode(26,OUTPUT);
  if(!SD.begin(26)){
    Serial.println("cannot connect to SD card!");
    while(1);
  }
  Serial.println("connected to SD card!");


  int c;
  cnt = SD.open("/cnt.txt");
  cnt.read((uint8_t*)&c, sizeof(c));
  c--;
  cnt.close();

  toRead = SD.open("/raw_accel_data_" + String(c) + ".txt");
  if(!toRead){
    Serial.printf("Could not open %s!\n", ("/raw_accel_data_" + String(c) + ".txt").c_str());
    while(1);
  }
  Serial.printf("Opened %s\n", ("/raw_accel_data_" + String(c) + ".txt").c_str());


  {
    File root = SD.open("/");
    File entry;

    while ((entry = root.openNextFile())) {
      Serial.printf("File '%s' (%u bytes)\n", entry.name(), entry.size());

      entry.close();
    }

    root.close();
  }

  delay(3000);


  int8_t accelNum;
  uint16_t n;
  toRead.seek(0);
  while(toRead.position() < toRead.size()){
    toRead.read((uint8_t*)&accelNum,sizeof(accelNum));
    toRead.read((uint8_t*)&n,sizeof(n));
    if(accelNum == 1){
      a1.push_back({toRead.position(),n});
    }
    else if(accelNum == 2){
      a2.push_back({toRead.position(),n});
    }
    else if(accelNum == 3){
      a3.push_back({toRead.position(),n});
    }
    else{
      a4.push_back({toRead.position(),n});
    }
    toRead.seek(toRead.position() + 6*n);
  }
  toRead.seek(0);


  int l1 = 0, l2 = 0, l3 = 0, l4 = 0;
  while(l1<a1.size() || l2<a2.size() || l3<a3.size() || l4<a4.size()){
    printData(l1,a1);
    Serial.printf(",");
    printData(l2,a2);
    Serial.printf(",");
    printData(l3,a3);
    Serial.printf(",");
    printData(l4,a4);
    Serial.printf("\n");
  }
}

void loop(){
}

void printData(int &loc, std::vector<dataLoc> &arr){
  int16_t x,y,z;
  if(loc >= arr.size()){
    Serial.printf(",,");
    return;
  }
  toRead.seek(arr[loc].loc);
  toRead.read((uint8_t*)&x,sizeof(x));
  toRead.read((uint8_t*)&y,sizeof(y));
  toRead.read((uint8_t*)&z,sizeof(z));
  Serial.printf("%0.4f,%0.4f,%0.4f",x*convRange64G,y*convRange64G,z*convRange64G);
  arr[loc].loc+=6;
  arr[loc].num--;
  if(arr[loc].num==0){
    loc++;
  }
}
