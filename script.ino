#include <SoftwareSerial.h>
SoftwareSerial BT_Serial(2, 3); // RX, TX

#include <IRremote.h>
const int RECV_PIN = A5;
IRrecv irrecv(RECV_PIN);
decode_results results;

#define enA 10 // Enable1 L298 Pin enA 
#define in1 9 // Right Motor forward L298 Pin in1 
#define in2 8 // Right Motor backward L298 Pin in1 
#define in3 7 // Left Motor forward L298 Pin in1 
#define in4 6 // Left Motor backward L298 Pin in1 
#define enB 5 // Enable2 L298 Pin enB 

#define servo A4

#define R_S A0 // IR sensor Right
#define L_S A1 // IR sensor Left

#define echo A2    // Echo pin
#define trigger A3 // Trigger pin

int distance_L, distance_F = 30, distance_R;
long distance;
int set = 20; // Set the distance to be 20cm

int bt_ir_data; // Variable to receive data from the serial port and IRremote
int Speed = 130; // Speed of the motor
int mode = 0; // Mode of working type

void setup() { // Put your setup code here, to run once

  pinMode(R_S, INPUT); // Declare if sensor as input
  pinMode(L_S, INPUT); // Declare ir sensor as input

  pinMode(echo, INPUT ); // declare ultrasonic sensor Echo pin as input
  pinMode(trigger, OUTPUT); // declare ultrasonic sensor Trigger pin as Output

  pinMode(enA, OUTPUT); // Declare as output for L298 Pin enA
  pinMode(in1, OUTPUT); // Declare as output for L298 Pin in1
  pinMode(in2, OUTPUT); // Declare as output for L298 Pin in2
  pinMode(in3, OUTPUT); // Declare as output for L298 Pin in3
  pinMode(in4, OUTPUT); // Declare as output for L298 Pin in4
  pinMode(enB, OUTPUT); // Declare as output for L298 Pin enB

  irrecv.enableIRIn(); // Start the receiver
  irrecv.blink13(true);

  Serial.begin(9600); // Start serial communication at 9600bps
  BT_Serial.begin(9600);

  pinMode(servo, OUTPUT);

  for (int angle = 70; angle <= 140; angle += 5)  {
    servoPulse(servo, angle);
  }
  for (int angle = 140; angle >= 0; angle -= 5)  {
    servoPulse(servo, angle);
  }

  for (int angle = 0; angle <= 70; angle += 5)  {
    servoPulse(servo, angle);
  }
  delay(500);
}


void loop() {

  if (BT_Serial.available() > 0) { // If some date is sent, reads it and saves in state
    bt_ir_data = BT_Serial.read();
    Serial.println(bt_ir_data);
    if (bt_ir_data > 20) {
      Speed = bt_ir_data;
    }
  }

  if (irrecv.decode()) {
    Serial.println(results.value, HEX);
    bt_ir_data = irRemoteData();
    Serial.println(bt_ir_data);
    irrecv.resume(); // Receive the next value
    delay(100);
  }

  if (bt_ir_data == 8) {
    mode = 0;  // Manual Android Application and IR Remote Control Command
    stop();
  }
  else if (bt_ir_data == 9) {
    mode = 1;  // Auto Line Follower Command
    Speed = 130;
  }
  else if (bt_ir_data == 10) {
    mode = 2;  // Auto Obstacle Avoiding Command
    Speed = 255;
  }

  analogWrite(enA, Speed); // Write The Duty Cycle 0 to 255 Enable Pin A for Motor1 Speed
  analogWrite(enB, Speed); // Write The Duty Cycle 0 to 255 Enable Pin B for Motor2 Speed

  if (mode == 0) {
    roboticCarControl();
  }
  else if (mode == 1) {
    lineFollowerControl();
  }
  else if (mode == 2) {
    obstacleAvoidingControl();
  }

  delay(10);
}

void roboticCarControl() {
  keyControlCommand();
  voiceControlCommand();
}

void keyControlCommand() {
  // ===============================================================================
  //                          Key Control Command
  // ===============================================================================
  if (bt_ir_data == 1) {
    forward(); // If the bt_data is '1' the DC motor will go forward
  }
  else if (bt_ir_data == 2) {
    backward(); // If the bt_data is '2' the motor will reverse
  }
  else if (bt_ir_data == 3) {
    turnLeft(); // If the bt_data is '3' the motor will turn left
  }
  else if (bt_ir_data == 4) {
    turnRight(); // If the bt_data is '4' the motor will turn right
  }
  else if (bt_ir_data == 5) {
    stop(); // If the bt_data '5' the motor will stop
  }
}

void voiceControlCommand() {
  // ===============================================================================
  //                          Voice Control Command
  // ===============================================================================
  if (bt_ir_data == 6) {
    turnLeft();
    delay(400);
    bt_ir_data = 5;
  }
  else if (bt_ir_data == 7) {
    turnRight();
    delay(400);
    bt_ir_data = 5;
  }
}

void lineFollowerControl() {
  // ===============================================================================
  //                          Line Follower Control
  // ===============================================================================

  if (digitalRead(L_S) == HIGH && digitalRead(R_S) == LOW) {
    turnRight();
  }
  else if (digitalRead(R_S) == HIGH && digitalRead(L_S) == LOW) {
    turnLeft();
  }
  else
    forward();
}

void obstacleAvoidingControl() {
  // ===============================================================================
  //                          Obstacle Avoiding Control
  // ===============================================================================
  distance_F = ultrasonicRead();
  Serial.print("S=");
  Serial.println(distance_F);
  if (distance_F > set) {
    forward();
  }
  else
    checkSide();
}

long irRemoteData() {
  int IR_data;

  if (results.value == 0xFF02FD) {
    IR_data = 1;
  }
  else if (results.value == 0xFF9867) {
    IR_data = 2;
  }
  else if (results.value == 0xFFE01F) {
    IR_data = 3;
  }
  else if (results.value == 0xFF906F) {
    IR_data = 4;
  }
  else if (results.value == 0xFF629D || results.value == 0xFFA857) {
    IR_data = 5;
  }
  else if (results.value == 0xFF30CF) {
    IR_data = 8;
  }
  else if (results.value == 0xFF18E7) {
    IR_data = 9;
  }
  else if (results.value == 0xFF7A85) {
    IR_data = 10;
  }

  return IR_data;
}

void servoPulse (int pin, int angle) {
  int pwm = (angle * 11) + 500;    // Convert angle to microseconds
  digitalWrite(pin, HIGH);
  delayMicroseconds(pwm);
  digitalWrite(pin, LOW);
  delay(50);                   // Refresh cycle of servo
}


//**********************ultrasonicRead****************************
long ultrasonicRead() {
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  distance = pulseIn (echo, HIGH);
  return distance / 29 / 2;
}

void compareDistance() {
  if (distance_L > distance_R) {
    turnLeft();
    delay(350);
  }
  else if (distance_R > distance_L) {
    turnRight();
    delay(350);
  }
  else {
    backward();
    delay(300);
    turnRight();
    delay(600);
  }
}

void checkSide() {
  stop();
  delay(100);
  for (int angle = 70; angle <= 140; angle += 5)  {
    servoPulse(servo, angle);
  }
  delay(300);
  distance_L = ultrasonicRead();
  delay(100);
  for (int angle = 140; angle >= 0; angle -= 5)  {
    servoPulse(servo, angle);
  }
  delay(500);
  distance_R = ultrasonicRead();
  delay(100);
  for (int angle = 0; angle <= 70; angle += 5)  {
    servoPulse(servo, angle);
  }
  delay(300);
  compareDistance();
}

void forward() {
  setMotorPins(HIGH, LOW, HIGH, LOW);
}

void backward() {
  setMotorPins(LOW, HIGH, LOW, HIGH);
}

void turnRight() {
  setMotorPins(LOW, HIGH, HIGH, LOW);
}

void turnLeft() {
  setMotorPins(HIGH, LOW, LOW, HIGH);
}

void stop() {
  setMotorPins(LOW, LOW, LOW, LOW);
}

void setMotorPins(int rightForward, int rightBackward, int leftForward, int leftBackward) {
  digitalWrite(in1, rightForward);
  digitalWrite(in2, rightBackward);
  digitalWrite(in3, leftForward);
  digitalWrite(in4, leftBackward);
}