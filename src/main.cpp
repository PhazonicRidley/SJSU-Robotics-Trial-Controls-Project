#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
const int MPU =  0x68;

Servo my_servo;
double starting_value = 90;
double servo_value = starting_value;
double raw_value;
const double LBS_to_g_constant = 8192; // using mode 1 for +-4gs for acceleration
const double LSB_to_deg_constant = 131;
bool running = false;

double positive_modulo(int number, int divisor)
{
  return (number % divisor + divisor) % divisor;
}

void serial_test_loop()
{
  Serial.print("Servo is at ");
  Serial.println(starting_value);
  if (Serial.available() > 0)
  {
    raw_value = Serial.readString().toInt();
    Serial.print("Got: ");
    Serial.println(raw_value);
    if (raw_value != 180)
      starting_value = positive_modulo((int)raw_value, 180);
    else
    starting_value = raw_value;
    //serv_value = map(raw_value, 0, 1023, 0, 180);
    my_servo.write(starting_value);
    delay(15);
  }
}

double rad_to_deg(double rad)
{
  return rad * 180 / PI;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  my_servo.attach(3);
  my_servo.write(90);  // let 90 degrees be "up" for the servo
  Wire.begin(); // TODO: refactor
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0); // wake sensor from sleep mode
  Wire.endTransmission(false);
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
  //Serial.println(running);
  if (cmd == String("stop\n"))
  {
    Serial.println("Stopped");
    running = false;
    my_servo.write(90);
    starting_value = 90;
  }
  else if (cmd == String("run\n"))
  {
    Serial.println("Running");
    running = true;
  }

  
  do 
  {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 4, true); // fetch raw data from i2c registers
  }
  while (Wire.available() == 0);
  raw_acc_x = Wire.read() << 8 | Wire.read(); // get x value from two registers
  raw_acc_y = Wire.read() << 8 | Wire.read(); // get y values from two registers
  do
  {
    Wire.beginTransmission(MPU);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 2, true);
  } while (Wire.available() == 0);
  raw_gy_x = Wire.read() << 8 | Wire.read();
  
  // calculate acceleration in x and y directions in gs
  acc_x = (double)raw_acc_x / LBS_to_g_constant;
  acc_y = (double)raw_acc_y / LBS_to_g_constant;  
  Serial.print("Accelerometer: X = " + String(acc_x));
  Serial.println(" | Y = " + String(acc_y));

  // calculate angular velocity in x and y directions in degrees/sec
  gy_x = (double)raw_gy_x / LSB_to_deg_constant;
  Serial.println("Gyroscope: X = " + String(gy_x));
  if (!running)
    return;

  /**
   * set needle using accelerometer data
   * Since 90 degrees is our "up", to stay up we add or subtract the calculated offset using
   * the accelerometer data.
   */

  // calculate offset angle
  double positional_angle = rad_to_deg(atan(acc_y/acc_x));
  Serial.println("Positional Offset: " + String(positional_angle) + " Degrees");
  // how far away from 90 degrees are we?
  auto new_serv_val = starting_value + round(positional_angle);
  // checks to make sure the new servo value isn't wrapping around. wont move servo if degree change is greater than 25, 
  // TODO: probably a superior way of doing this
  if (abs(new_serv_val - servo_value) < 100) 
  {
    my_servo.write(new_serv_val);
    servo_value = new_serv_val;
  }
  Serial.println("Position: " + String(servo_value) + " Degrees\n");
  


}

