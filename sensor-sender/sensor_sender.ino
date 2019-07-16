//#include <SoftwareSerial.h>
#include <dht.h>  // https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable
#include <tinysnore.h> // https://github.com/connornishijima/TinySnore
#include <RCSwitch.h> // https://github.com/Martin-Laclaustra/rc-switch/tree/protocollessreceiver

//SoftwareSerial mySerial(3, 4); // RX, TX
dht DHT;
RCSwitch rfData = RCSwitch();
#define DHT11_PIN 2 //attiny PB2 to DHT DATA 
int transmission_interval = 180;
int v_min;
int packet_id = 0; //8bits from 0 to 7
int sensor_id = 1; //6bits, 64 sensors max, interval 0 - 63, change it at every sensor-sender unit :)
void setup()
{
//  mySerial.begin(9600);
  pinMode(1, OUTPUT); //to transmitter module DATA pin 
  pinMode(0, OUTPUT); //power for transmitter and DHT11
  rfData.setProtocol(0);
  rfData.setRepeatTransmit(4); //minimum repeat transmit
  rfData.setPulseLength(200); //is working as low as 120us but link is not reliable
  rfData.enableTransmit(1); //data on attiny PB1
}

void loop()
{
  digitalWrite(0, HIGH); //power to sensor and radio module
  delay(200); //wait for sensor to wake-up
//  mySerial.print("DHT11, \t");
  int chk = DHT.read11(DHT11_PIN); //not using but can be usefull as in commented part
//  switch (chk)
//  {
//    case DHTLIB_OK: 
//                           //mySerial.print("OK,\t");
//                           break;
//    case DHTLIB_ERROR_CHECKSUM:
//                           //mySerial.print("Checksum error,\t");
//                           break;
//    case DHTLIB_ERROR_TIMEOUT:
//                           //mySerial.print("Time out error,\t");
//                           break;
//    default:
//                           //mySerial.print("Unknown error,\t");
//                           break;
//  }
//if (chk != 0) { //if no value red from sensor
//  DHT.temperature = 100;
//  DHT.humidity = 0;
//}
//  // DISPLAY DATA
//  mySerial.print(DHT.humidity);
//  mySerial.print(",\t");
//  mySerial.println(DHT.temperature);
  int volt = getVcc(); //measuring the battery voltage
//  mySerial.println(volt);
  if (volt < 3300) { //if less then bit v_min true
    v_min = 1;
  }
  double l; //rate parameter. see https://preshing.com/20111007/how-to-generate-random-timings-for-a-poisson-process/
  l = 1/(double)transmission_interval;
  double k = (ran_expo(l)*1000); //generate the interval for poisson process
//  mySerial.println(k);
  unsigned long data = 0;
  data = DHT.humidity + 128*DHT.temperature + v_min*16384 + packet_id*32768 + sensor_id*262144;
  //data to be transmitted is like this: from less significant bit - 7bits humidity + 7bits temperature + 1bit v_min + 3bits packet_id + 6bits sensor_id
  // s      p   v 25      45
  // 010000 011 0 0011101 0101101  of course can be coded as you need
  //data = 45 + 128*25 + 0*16384 + 3*32768 + 16*262144;
  //mySerial.println(data);
  String bin_data;
  bin_data = long2bin(data); //transform the unsigned long data to a binary values string
  //mySerial.println(bin_data);
  char bin_array[bin_data.length()+1];
  bin_data.toCharArray(bin_array, bin_data.length()+1); //to send by rcswitch string must be transformed in character array
  //mySerial.println(bin_array);+
  rfData.send(bin_array); //send 24bits - with protocol 1 the radio burst length will be aprox. 102.4 ms but in reality the
  // pulse length is about 213us so the radio burst will be 110ms
  digitalWrite(0, LOW); //stop power at sensor and radio module
  packet_id ++; //count up the packet id
  if (packet_id == 8) {
    packet_id = 0;
  }
  //delay(1000); //for testing
  snore(k); //deep sleep for a random interval (but generated for poisson process)
}

//functin to generate timing for poisson process 
double ran_expo(double lambda)
{
  double u;
  u = rand() / (RAND_MAX + 1.0);
    return -log(1 - u) / lambda;
}
//function to look for battery voltage - inspired by http://oliverwalkhoff.com/cocolora
long getVcc() {
  // read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(MUX3) | _BV(MUX2);
  delay(2); // wait for Vref to settle
  ADCSRA |= _BV(ADSC); // start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;
  result = 1126400L / result; // calculate Vcc (in mV); 1126400 = 1.1*1024*1000 // generally                     
  return (int) result;
}
//function to transform decimal to binary string - inspired by https://stackoverflow.com/questions/29983943/converting-int-to-binary-in-c-arduino#
String long2bin (unsigned long nb)
  {
  int   i = 23;
  String  bin = "";
  while (i >= 0) {
    if ((nb >> i) & 1)
      bin = bin + "1";
    else
      bin = bin + "0";
    --i;
  }
  return bin;

}
