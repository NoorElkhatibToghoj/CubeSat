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
class TimeStamp
{
  // These variables use the Short data type
  // See: https://www.arduino.cc/en/Reference/Short
  short year;
  short month;
  short day;
  short hour;
  short minute;
  short seconds;

  TimeStamp(short year, short month, short day, short minute, short seconds)
  {
    this.year = year;
    this.month = month;
    this.day = day;
    this.minute = minute;
    this.seconds = seconds;
  }
};

// =======================================
// Power Data Structure
// Handling data from EPS and from on board power on OBC
// =======================================
class PowerData
{
  float voltage;
  float current;

  // ================================================
  // Default Constructor for the Power Data Structure
  // ================================================
  PowerData(float voltage, float current)
  {
    this.voltage = voltage;
    this.current = current;
  }
};

// =======================================
// Beacon Data Structure
// Handling data that will be communicated through morse code
// =======================================
class BeaconData
{
  string callSign;
  string message;
  int messageLength;

  BeaconData (string callSign,
  string message,
  int messageLength)
  {
    this.callSign = callSign;
    this.message = message;
    this.messageLength = messageLength;
  }
};

// =======================================
// Telemetry Data Structure
// Data about the cubesat to be stored on the OBC
// and transmitted to ground station
// =======================================
class TelemetryData
{
  TimeStamp TS;
  PowerData PD;

  short id; // counter, sequence number of the telemetry data entry
  bool payloadAlive;
  float systemTemperature;

  // ====================================================
  // Default Constructor for the Telemetry Data Structure
  // ====================================================
  TelemetryData(short id, bool payloadAlive, float systemTemperature)
  {
    this.id = id;
    this.payloadAlive = payloadAlive;
    this.systemTemperature = systemTemperature;
  }

  TelemetryData(TelemetryData copyConstructor)
  {
    this.id = copyConstructor.id;
    this.payloadAlive = copyConstructor.payloadAlive;
    this.systemTemperature = copyConstructor.systemTemperature;

    this.TS = copyConstructor.getTimeStamp();
    this.PD = copyConstructor.getPowerData();
  }

  void setTimeStamp(TimeStamp TS)
  {
    this.TS = TS;
  }

  TimeStamp getTimeStamp()
  {
    return this.TS;
  }

  void setPowerData(PowerData PD)
  {
    this.PD = PD;
  }

  PowerData getPowerData()
  {
    return this.PD;
  }
};

// =======================================
// Payload Data Structure
// Data structure to rx and tx with the payload
// =======================================
class PayloadData
{
  bool TxOrRx;
  string TXmessage;

  bool TXready;
  bool RXready;

  // ====================================================
  // Default Constructor for the Telemetry Data Structure
  // ====================================================
  PayloadData (bool TxOrRx, string TXmessage)
  {
    this.TxOrRx = TxOrRx;
    this.TXmessage = TXmessage;
  }

  void setTXready(bool TXready)
  {
    this.TXready = TXready;
  }

  void setRXready(bool RXready)
  {
    this.RXready = RXready;
  }
};



// =======================================
// On Board Computer Data Class
// Data structure for the OBC
// =======================================
class OBCData
{
  bool antennaDeployed;
  bool postLaunchComplete;
  int currentState;
  bool telemetryRequests;

  // ====================================================
  // Default Constructor for the Telemetry Data Structure
  // ====================================================
  OBCData (bool antennaDeployed,
    bool postLaunchComplete,
    int currentState,
    bool telemetryRequests)
  {
    this.antennaDeployed = antennaDeployed;
    this.postLaunchComplete = postLaunchComplete;
    this.currentState = currentState;
    this.telemetryRequests = telemetryRequests;
  }
};


// =======================================
// General Data Class
// =======================================
class Data
{
  public:
    TelemetryData telData[3];
    int telCount;

    PayloadData payData[3];
    int payCount;

    OBCData obcData[3];
    int obcCount;

    // ====================================================
    // Default Constructor for the General Data class
    // ====================================================
    Data ()
    {
      telCount = -1;
      payCount = -1;
      obcCount = -1;

      // make stack sizes dynamic using malloc or something similar
    }

    // ======================================================
    // Add Telemetry Entry function inside General Data class
    // ======================================================
    void addTelemetryEntry (TelemetryData t)
    {
      if ((telCount + 1) < 3)
      {
        telCount++;
        telData[telCount] = t;
      }
      else
      {
        for (int i = 1; i < 3; i++)
        {
          telData[i - 1] = telData[i];
        }
        telData[telCount] = t;
      }
    }

    // =====================================================
    // General Data Class: Adding Payload Entry function
    // =====================================================
    void addPayloadEntry (PayloadData p)
    {
      if ((payCount + 1) < 3)
      {
        payCount++;
        payData[payCount] = p;
      }
      else
      {
        for (int i = 1; i < 3; i++)
        {
          payData[i - 1] = payData[i];
        }
        payData[payCount] = p;
      }
    }

    // =====================================================
    // General Data Class: Adding OBC Entry function
    // =====================================================
    void addOBCEntry (OBCData o)
    {
      if ((obcCount + 1) < 3)
      {
        obcCount++;
        obcData[obcCount] = o;
      }
      else
      {
        for (int i = 1; i < 3; i++)
        {
          obcData[i - 1] = obcData[i];
        }
        obcData[obcCount] = o;
      }
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

    // ====================================================
    // General Data Class: Deleting a OBC Row function
    // ====================================================
    void deleteOBCRow()
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

      orbitalModeDelay(); // Sleep for 15 min for other subsystems
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
