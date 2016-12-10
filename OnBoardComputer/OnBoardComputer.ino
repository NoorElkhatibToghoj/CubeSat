#include <SimpleTimer.h>
#include <Wire.h>
#include <string.h>

#define PostLaunch 0
#define Beaconing 1
#define Recharge 2
#define Failsafe 3
#define Telecom 4

// ???
#define temp_add 90 // assume 90
#define xyz  100 // assume 100

// define addresses for subsystems

// check maximum time for the timer
SimpleTimer timer;

// make a class for temperature sensor and add function to read temp from sensor
void getTemperature ()
{
  Wire.beginTransmission(temp_add); //start talking
  Wire.write(0); //ask for reg 0
  Wire.endTransmission(); // complete talking
  Wire.requestFrom(temp_add, 1); // ask for 1 byte
  while (Wire.available() == 0); // wait for response
  float c = Wire.read(); // get the temp
  float f = round(c * 9.0 / 5.0 + 32.0);
  Serial.print(c);
  Serial.print("C, ");
  Serial.print(f);
  Serial.print("F.");
  delay (3000);
}

// =======================================
// ADCS Function
// convert into class
// =======================================
void ADCS ()
{
  // Define ADCS data structure
  int x, y, z;

  Wire.beginTransmission (xyz); // xyz = ADCS address
  Wire.write("start");
  Serial.println("Starting ADCS Transmission");
  Wire.endTransmission();
  delay(500);
  Wire.beginTransmission(xyz);

  Wire.write(5); // assume x
  Wire.write(6); // assume y
  Wire.write(7); // assume zp

  Wire.endTransmission();

  Wire.requestFrom(xyz, 3);

  if (Wire.available() <= 3)
  {
    x = Wire.read();
    y = Wire.read();
    z = Wire.read();
  }

  Serial.print("X =");
  Serial.print(x);
  Serial.print("Y =");
  Serial.print(y);
  Serial.print("Z =");
  Serial.print(z);

  // enter x,y,z into cubesat data
}

// =======================================
// Communication Class
// Unification of I2C and Serial communication
// =======================================
class Communication
{
    bool I2C;
    bool serial;

    // ========================================
    // Default Communication Class constructor
    // ========================================
    Communication ()
    {

    }

    bool startCommunication()
    {
      return true;
    }
};

// =======================================
// TimeStamp Structure
// From the on board clock on the OBC
// =======================================
struct TimeStamp
{
  // change to unsigned short or unsigned char
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int seconds;

  TimeStamp() // change constructor
  {
    hour = 12;
    minute = 30;
    seconds = 20;
  }
};

// =======================================
// Power Data Structure
// Handling data from EPS and from on board power on OBC
// =======================================
struct PowerData
{
  int voltage;
  int current;

  // ================================================
  // Default Constructor for the Power Data Structure
  // ================================================
  PowerData()
  {
    voltage = 5;
    current = 0.133;
  }

  // TODO: Bus currents (5 and 3.3 buses)

};

// =======================================
// Beacon Data Structure
// Handling data that will be communicated through morse code
// =======================================
struct BeaconData
{
  string callSign;
  string message;
  int messageLength;

  // continue
};

// =======================================
// Telemetry Data Structure
// Data about the cubesat to be stored on the OBC
// and transmitted to ground station
// =======================================
struct TelemetryData
{
  TimeStamp TS;
  PowerData PD;

  // research unsigned short and unsigned char
  
  int id; // counter, sequence number of the telemetry data entry
  int telemetryTime; // ???
  int batteryLevel; // remove after checking
  bool payloadAlive;
  int systemTemperature;
  

  // ====================================================
  // Default Constructor for the Telemetry Data Structure
  // ====================================================
  TelemetryData() // insert into constructor definition
  {
    batteryLevel = 100;
    payloadAlive = false;
    systemTemperature = 25;
  }

};

// =======================================
// Payload Data Structure
// Data structure to rx and tx with the payload
// =======================================
struct PayloadData
{
  bool TxOrRx;
  string TXmessage;
  int beaconInterval; // move to beacon data structure 
  string callsign;

  // research if we should add health data

  // make constructor
};

// =======================================
// General Data Class
// 
// =======================================
class Data
{
  public:
    TelemetryData telData[3];
    int telCount;

    PayloadData payData[3];
    int payCount;

    // create data structure for OBCData to determine the current state
    bool antennaDeployed;
    bool TXready; // move inside payload data
    bool RXready; // ``
    bool postLaunchComplete;
    int currentState;
    bool telemetryRequests;


    // ====================================================
    // Default Constructor for the General Data class
    // ====================================================
    Data()
    {
      telCount = -1;
      // make stack sizes dynamic using malloc or something similar
    }

    // ====================================================
    // View Status function for the General Data class
    // ====================================================
    void ViewStatus()
    {
      if (telCount == -1)
      {
        Serial.print("No entries!");
      }
      else
      {
        Serial.print("Battery Level");
        Serial.print(telData[telCount].batteryLevel);

        Serial.print("Temperature");
        Serial.print(telData[telCount].systemTemperature);

        Serial.print("Time");
        Serial.print(telData[telCount].TS.hour);
        Serial.print(telData[telCount].TS.minute);
        Serial.print(telData[telCount].TS.seconds);

        Serial.print("Payload Alive");
        Serial.print(telData[telCount].payloadAlive);
        Serial.print("Payload Voltage");
        Serial.print(telData[telCount].PD.voltage);
        Serial.print("Payload Current");
        Serial.print(telData[telCount].PD.current);
      }
    }

    // ======================================================
    // Add Telemetry Entry function inside General Data class
    // TODO: copy constructors for all classes to simplify stack 
    // ======================================================
    void AddTelemetryEntry(TelemetryData t)
    {
      if ((telCount + 1) < 3)
      {
        telCount++;

        telData[telCount].batteryLevel = t.batteryLevel;
        telData[telCount].id = t.id;
        telData[telCount].payloadAlive = t.payloadAlive;
        telData[telCount].PD.current = t.PD.current;
        telData[telCount].PD.voltage = t.PD.voltage;
        telData[telCount].systemTemperature = t.systemTemperature;
        telData[telCount].telemetryTime = t.telemetryTime;
        telData[telCount].TS.day = t.TS.day;
        telData[telCount].TS.hour = t.TS.hour;
        telData[telCount].TS.minute = t.TS.minute;
        telData[telCount].TS.month = t.TS.month;
        telData[telCount].TS.seconds = t.TS.seconds;
        telData[telCount].TS.year = t.TS.year;
      }
      else
      {
        //tx the last entry
        for (int i = 1; i < 3; i++)
        {
          telData[i - 1].batteryLevel = telData[i].batteryLevel;
          telData[i - 1].id = telData[i].id;
          telData[i - 1].payloadAlive = telData[i].payloadAlive;
          telData[i - 1].PD.current = telData[i].PD.current;
          telData[i - 1].PD.voltage = telData[i].PD.voltage;
          telData[i - 1].systemTemperature = telData[i].systemTemperature;
          telData[i - 1].telemetryTime = telData[i].telemetryTime;
          telData[i - 1].TS.day = telData[i].TS.day;
          telData[i - 1].TS.hour = telData[i].TS.hour;
          telData[i - 1].TS.minute = telData[i].TS.minute;
          telData[i - 1].TS.month = telData[i].TS.month;
          telData[i - 1].TS.seconds = telData[i].TS.seconds;
          telData[i - 1].TS.year = telData[i].TS.year;
        }

        telData[telCount].batteryLevel = t.batteryLevel;
        telData[telCount].id = t.id;
        telData[telCount].payloadAlive = t.payloadAlive;
        telData[telCount].PD.current = t.PD.current;
        telData[telCount].PD.voltage = t.PD.voltage;
        telData[telCount].systemTemperature = t.systemTemperature;
        telData[telCount].telemetryTime = t.telemetryTime;
        telData[telCount].TS.day = t.TS.day;
        telData[telCount].TS.hour = t.TS.hour;
        telData[telCount].TS.minute = t.TS.minute;
        telData[telCount].TS.month = t.TS.month;
        telData[telCount].TS.seconds = t.TS.seconds;
        telData[telCount].TS.year = t.TS.year;
      }
    }

    // =====================================================
    // General Data Class: Adding Payload Entry function
    // =====================================================
    void addPayloadEntry (PayloadData p)
    {

    }

    // =====================================================
    // General Data Class: Deleting a Telemetry Row function
    // =====================================================
    void deleteTelemetryRow()
    {

    }

    // ====================================================
    // General Data Class: Deleting a Payload Row function
    // ====================================================
    void deletePayloadRow()
    {

    }
};

// ====================================================
// Cubesat States Class
// ====================================================
class States
{
  public:
    int currentState;

    // ====================================================
    // Changing the Current Cubesat State function
    // ====================================================
    void changeCurrentState(Data data)
    {
      if (data.postLaunchComplete)
      {
        if (data.batteryLevel > 0.6 && !data.telemetryRequests)
        {
          currentState = Beaconing;
        }
        else if (data.batteryLevel > 0.9 &&  data.telemetryRequests)
        {
          currentState = Telecom;
        }
        else if (data.batteryLevel < 0.6 && data.batteryLevel > 0.2)
        {
          currentState = Recharge;
        }
        else if (data.batteryLevel < 0.2)
        {
          currentState = Failsafe;
        }
      }
      else
      {
        currentState = PostLaunch;
      }
    }

    // ====================================================
    // Default States class constructor
    // ====================================================
    States()
    {
      currentState = 0;
    }
};

// ====================================================
// General Cubesat Class
// ====================================================
class Cubesat
{
  public:
    States JY1STATES;
    Data JY1DATA;
    Communication com;

    // ====================================================
    // Default Constructor for the General Cubesat Class
    // ====================================================
    Cubesat()
    {
      // check if current state is post launch
      JY1DATA.antennaDeployed = true;
      JY1DATA.postLaunchComplete = true;
      JY1DATA.currentState = PostLaunch;
      JY1DATA.telemetryRequests = false;
      JY1DATA.batteryLevel = 0.9;
    }

    // ====================================================
    // Entering Orbital Mode Delay function
    // 1. Send sleep transmission to all controllers
    // 2. Wait 15 minutes
    // ====================================================
    void orbitalModeDelay ()
    {
      Wire.beginTransmission(98); // transmit to device's
      Wire.write("Sleep");        // sends five bytes
      Wire.endTransmission();    // stop transmitting
      Serial.print("orbit mode");

      // timer does 15 mintues sleep
    }

    // ====================================================
    // Checking the Radio Frequency function
    // Check if payload is alive; return status of RF payload
    // ====================================================
    int checkRadioFrequency ()
    {
      Wire.beginTransmission(8); // transmit to device #8
      Wire.write("Start");        // sends five bytes
      Serial.println("Starting Transmission");
      Wire.endTransmission();    // stop transmitting

      delay(500);

      Wire.beginTransmission(8); // transmit to device #8
      Wire.write("JY1SAT TEST");
      Serial.println("JY1SAT TEST");
      Wire.endTransmission();    // stop transmitting

      delay(1000);

      Wire.beginTransmission(8); // transmit to device #8
      Wire.write("Stop");
      Serial.println("Stopping Transmission");
      Wire.endTransmission();    // stop transmitting
      
      delay(1000);
    }

    // ====================================================
    // Checking the Battery function
    // Return the EPS state as an EPS class data structure
    // ====================================================
    int checkBattery ()
    {
      Wire.beginTransmission(78); // transmit to eps's
      Wire.write("START");        // sends five bytes
      Wire.endTransmission();     // stop transmitting
      Wire.beginTransmission(77);
      Wire.write(5);              // assume x
      Wire.endTransmission();
      Wire.requestFrom(80, 3);    // ASSUME EPS 80
      if (Wire.available() <= 1)
      {
        JY1DATA.batteryLevel = Wire.read();
      }

      Serial.print("BATTERY LEVEL = ");
      Serial.print(JY1DATA.batteryLevel);
    }

    // ====================================================
    // Cubesat Post Launch function
    // ====================================================
    void postLaunch()
    {
      timer.run();

      orbitalModeDelay();              // Sleep for 15 min for preperation of other subsystems
      checkBattery ();
      checkRadioFrequency ();
    }

    // ====================================================
    // Cubesat Fail Safe State function
    // ====================================================
    void failSafe()
    {
      Wire.beginTransmission(8); // transmit to device #8
      Wire.write("Sleep");        // sends five bytes
      Wire.endTransmission();    // stop transmitting
    }

    // ====================================================
    // Cubesat Recharging State function
    // ====================================================
    void recharging()
    {

    }

    // ====================================================
    // Cubesat Beaconing State Function
    // ====================================================
    void beaconing()
    {
      Serial.println("We are in beaconing mode");
      char x = 's';
      Wire.beginTransmission(8); // transmit to device #8
      Wire.write("Start");        // sends five bytes
      Serial.println("Starting Beaconing");
      Wire.endTransmission();    // stop transmitting
      delay(500);

      Wire.beginTransmission(8); // transmit to device #8
      
      Wire.write("JY1SAT TEST");
      Serial.println("JY1SAT TEST");
      Wire.endTransmission();    // stop transmitting

      delay(1000);

      Wire.beginTransmission(8); // transmit to device #8
      Wire.write("Stop");
      Serial.println("Stop .. ");
      Wire.endTransmission();    // stop transmitting
      
      delay(1000);
      
      Wire.requestFrom(8, 50);    // request 6 bytes from slave device #8

      while (Wire.available()) 
      { 
        // slave may send less than requested
        char c = Wire.read(); // receive a byte as character
        Serial.print(c);         // print the character
      }
    }

    // ====================================================
    // Cubesat Telecommunications Function
    // ====================================================
    void telecom()
    {
      
    }
};

// ====================================================
// Arduino: Global Variables Initializations
// ====================================================
Cubesat JY1SAT;
TelemetryData tt;

// ====================================================
// Arduino: Setup Function
// ====================================================
void setup()
{
  Wire.begin();
  Serial.begin(9600);
}

// ====================================================
// Arduino: Loop Function
// ====================================================
void loop()
{
  JY1SAT.JY1STATES.changeCurrentState(JY1SAT.JY1DATA);
  switch (JY1SAT.JY1STATES.currentState)
  {
    case PostLaunch:
      JY1SAT.postLaunch();
      break;

    case Failsafe:
      JY1SAT.failSafe();
      break;

    case Telecom:
      JY1SAT.telecom();
      break;

    case Beaconing:
      JY1SAT.beaconing();
      break;

    case Recharge:
      JY1SAT.recharging();
      break;

    default:
      break;
  }
}



