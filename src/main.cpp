#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
const int MPU =  0x68;

Servo my_servo;
double serv_value = 90;
double raw_value;
const double LBS_to_g_constant = 8192;
bool running = false;

double positive_modulo(int number, int divisor)
{
  return (number % divisor + divisor) % divisor;
}

void serial_test_loop()
{
  Serial.print("Servo is at ");
  Serial.println(serv_value);
  if (Serial.available() > 0)
  {
    raw_value = Serial.readString().toInt();
    Serial.print("Got: ");
    Serial.println(raw_value);
    if (raw_value != 180)
      serv_value = positive_modulo((int)raw_value, 180);
    else
    serv_value = raw_value;
    //serv_value = map(raw_value, 0, 1023, 0, 180);
    my_servo.write(serv_value);
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
  Wire.endTransmission(true);
}

void loop() {
  // put your main code here, to run repeatedly:
  int16_t raw_acc_x, raw_acc_y;
  String cmd;
  double acc_x, acc_y;
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
    serv_value = 90;
  }
  else if (cmd == String("run\n"))
  {
    Serial.println("Running");
    running = true;
  }

  if (!running)
    return;

  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.write(0x1C); // set a range of +- 4g for acceleration measurement
  Wire.write(1);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 4, true); // fetch raw data from i2c registers
  
  raw_acc_x = Wire.read() << 8 | Wire.read(); // get x value from two registers
  raw_acc_y = Wire.read() << 8 | Wire.read(); // get y values from two registers
  acc_x = (double)raw_acc_x / LBS_to_g_constant;
  acc_y = (double)raw_acc_y / LBS_to_g_constant;
  Serial.print("X = ");
  Serial.print(acc_x);
  Serial.print(" Y = ");
  Serial.println(acc_y);

  /**
   * set needle using accelerometer data
   * Since 90 degrees is our "up", to stay up we add or subtract the calculated offset using
   * the accelerometer data.
   */

  // calculate offset angle
  double positional_angle = rad_to_deg(atan2(acc_y, acc_x));
  // how far away from 90 degrees are we?
  //serv_value += offset_angle;
  //double angle = positive_modulo(offset_angle, 180);
  my_servo.write(round(positional_angle));
  Serial.print("\nAngle difference angle: ");
  Serial.println(90 - positional_angle);
  Serial.println("\nPositional angle: " + String(positional_angle));
  // TODO: try with rotation and gyroscope
  //delay(10);


}

