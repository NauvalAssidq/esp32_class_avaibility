#include <Wire.h>
#include <VL53L0X.h>
#define XSHUT_PIN1 13
#define XSHUT_PIN2 12
#define LOX1_ADDRESS 0x30
#define LOX2_ADDRESS 0x31

VL53L0X sensor1;
VL53L0X sensor2;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pinMode(XSHUT_PIN1, OUTPUT);
  pinMode(XSHUT_PIN2, OUTPUT);
  digitalWrite(XSHUT_PIN1, LOW);
  digitalWrite(XSHUT_PIN2, LOW);
  delay(10);
  digitalWrite(XSHUT_PIN1, HIGH);
  delay(10);
  
  sensor1.setAddress(LOX1_ADDRESS);
  if (!sensor1.init()) {
    Serial.println("Failed to detect and initialize sensor 1!");
    while (1);
  }

  digitalWrite(XSHUT_PIN2, HIGH);
  delay(10);

  sensor1.setAddress(LOX1_ADDRESS);
  sensor2.setAddress(LOX2_ADDRESS);
  if (!sensor2.init()) {
    Serial.println("Failed to detect and initialize sensor 2!");
    while (1);
  }

  sensor1.setMeasurementTimingBudget(200000);
  sensor2.setMeasurementTimingBudget(200000);
  sensor1.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor1.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  
  sensor2.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor2.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  sensor1.startContinuous(0);
  sensor2.startContinuous(0);

  Serial.println("Both sensors initialized in High Accuracy mode.");
}

void loop() {
  uint16_t distance1 = sensor1.readRangeContinuousMillimeters();
  uint16_t distance2 = sensor2.readRangeContinuousMillimeters();

  Serial.print("S1: ");
  if (sensor1.timeoutOccurred()) {
    Serial.print("TIMEOUT");
  } else if (distance1 > 600 || distance1 >= 8190) { 
    Serial.print(">600mm (Out of Bounds)");
  } else {
    Serial.print(distance1);
    Serial.print("mm");
  }

  Serial.print(" | ");

  Serial.print("S2: ");
  if (sensor2.timeoutOccurred()) {
    Serial.print("TIMEOUT");
  } else if (distance2 > 600 || distance2 >= 8190) {
    Serial.print(">600mm (Out of Bounds)");
  } else {
    Serial.print(distance2);
    Serial.print("mm");
  }

  Serial.println();

  delay(100);
}
