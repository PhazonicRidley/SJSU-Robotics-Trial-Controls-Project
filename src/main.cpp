#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#define MPU 0x68

Servo my_servo;
double initial_position = 90; // initial value, where "up" is defined as.
double servo_value = initial_position;
const double LBS_to_g_constant = 8192; // using mode 1 for +-4gs for acceleration
bool running = false;

// takes the positive remainder of a number given a divisor, intended to be used to "loop around" a number.
int positive_modulo(int number, int divisor)
{
  return (divisor + (number % divisor)) % divisor;
}

// converts radians to degrees.
double rad_to_deg(double rad)
{
  return rad * 180 / PI;
}

void setup() {
  Serial.begin(115200); // configure Serial communication, higher baudrate to allow the system to run better
  // configure servo
  my_servo.attach(3);
  my_servo.write(initial_position); 

  // Configure i2c bus and configure GY 521 
  Wire.begin(); 
  // wake sensor from sleep mode
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0); 
  Wire.endTransmission(false);
  // Set the scale of the accelerometer to be +- 4g 
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);
  Wire.write(1); 
  Wire.endTransmission(true);
}

// Used to change the starting position of the servo needle, sets where "up" is.
void set_starting_offset(String s)
{
  int offset = s.toInt() % 90;
  if (offset == 0)
    initial_position = 90;
  else
    initial_position = positive_modulo(offset, 180);
}

void loop() {
  int16_t raw_acc_x, raw_acc_y, raw_acc_z;
  String cmd = "";
  double acc_x, acc_y, acc_z;

  // read inputs
  if (Serial.available() > 0)
  {
    cmd = Serial.readString();
    cmd.trim();
  }
  if (cmd == String("stop"))
  {
    Serial.println("Stopped");
    running = false;
    my_servo.write(90);
    initial_position = 90;
  }
  else if (cmd == String("run"))
  {
    Serial.println("Running");
    running = true;
  }

  if (!running)
    return;
  
  if (cmd != "")
    set_starting_offset(cmd);

  // retrieve accelerometer data
  do 
  {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true); // read 6 bytes of data for x, y, and z
  }
  while (Wire.available() == 0);
  raw_acc_x = Wire.read() << 8 | Wire.read(); // get x value from two registers
  raw_acc_y = Wire.read() << 8 | Wire.read(); // get y values from two registers
  raw_acc_z = Wire.read() << 8 | Wire.read(); // get z values from two registers
  
  // calculate acceleration in x and y directions in g
  acc_x = (double)raw_acc_x / LBS_to_g_constant;
  acc_y = (double)raw_acc_y / LBS_to_g_constant; 
  acc_z = (double)raw_acc_z / LBS_to_g_constant; 
  Serial.print("Acceleration Force: X = " + String(acc_x) + " | Y = " + String(acc_y));
  Serial.println(" | Z = " + String(acc_z));

  // calculate positional angle with respect to the x and y vectors of acceleration
  double positional_angle = rad_to_deg(atan(acc_y / acc_x));
  Serial.println("Positional correction angle: " + String(positional_angle) + " Degrees");
  Serial.println("Initial position: ('Up' is...) " + String(initial_position) + " Degrees");
  // offset needle from our starting set starting position
  auto new_serv_val = initial_position + round(positional_angle);
  // checks to make sure the new servo value isn't wrapping around. wont move servo if degree change is greater than 100
  if (abs(new_serv_val - servo_value) < 120) 
  {
    my_servo.write(new_serv_val);
    servo_value = new_serv_val;
  }  

}

