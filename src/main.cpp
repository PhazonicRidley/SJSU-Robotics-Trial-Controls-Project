#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#define MPU 0x68

Servo my_servo;
double initial_position = 90; // initial value
double servo_value = initial_position;
const double LBS_to_g_constant = 8192; // using mode 1 for +-4gs for acceleration
const double LSB_to_deg_constant = 131; // using mode 0 for +- 250 deg/sec for gyroscope
bool running = false;

double rad_to_deg(double rad)
{
  return rad * 180 / PI;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // configure Serial communication
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

void loop() {
  // put your main code here, to run repeatedly:
  int16_t raw_acc_x, raw_acc_y, raw_gy_x;
  String cmd;
  double acc_x, acc_y, gy_x;
  if (Serial.available() > 0)
  {
    cmd = Serial.readString();
    Serial.println(cmd);
  }
  if (cmd == String("stop\n"))
  {
    Serial.println("Stopped");
    running = false;
    my_servo.write(90);
    initial_position = 90;
  }
  else if (cmd == String("run\n"))
  {
    Serial.println("Running");
    running = true;
  }

  if (!running)
    return;

  // retrieve accelerometer data
  do 
  {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 4, true); // read 4 bytes of data for x and y
  }
  while (Wire.available() == 0);
  raw_acc_x = Wire.read() << 8 | Wire.read(); // get x value from two registers
  raw_acc_y = Wire.read() << 8 | Wire.read(); // get y values from two registers

  // retrieve gyroscope data
  do
  {
    Wire.beginTransmission(MPU);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 2, true); // read 2 bytes for just x
  } while (Wire.available() == 0);

  raw_gy_x = Wire.read() << 8 | Wire.read(); // parse data
  
  // calculate acceleration in x and y directions in g
  acc_x = (double)raw_acc_x / LBS_to_g_constant;
  acc_y = (double)raw_acc_y / LBS_to_g_constant;  
  Serial.print("Accelerometer: X = " + String(acc_x));
  Serial.println(" | Y = " + String(acc_y));

  // calculate angular velocity in x and y directions in degrees/sec
  gy_x = (double)raw_gy_x / LSB_to_deg_constant;
  Serial.println("Gyroscope: X = " + String(gy_x));


  // calculate positional angle with respect to the x and y vectors of acceleration
  double positional_angle = rad_to_deg(atan(acc_y/acc_x));
  Serial.println("Offset angle: " + String(positional_angle) + " Degrees");
  // offset needle from our starting set starting position
  auto new_serv_val = initial_position + round(positional_angle);
  // checks to make sure the new servo value isn't wrapping around. wont move servo if degree change is greater than 100
  if (abs(new_serv_val - servo_value) < 100) 
  {
    my_servo.write(new_serv_val);
    servo_value = new_serv_val;
  }
  Serial.println("Position: " + String(servo_value) + " Degrees\n");
  


}

