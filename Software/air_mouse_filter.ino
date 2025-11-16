#include <Wire.h>
#include <MPU6050.h>
#include <BleMouse.h>
int filterIndex = 0;  

#define SCROLL_BUTTON_PIN 33  // GPIO 4 for Scroll Button
#define FILTER_SIZE 5  // Size of Moving Average Filter 
#define BUTTON_PIN 4  // GPIO 4 for right-click button
#define FLEX_PIN 32   // GPIO 32 for flex sensor (analog input)

int xHistory[FILTER_SIZE] = {0};  
int yHistory[FILTER_SIZE] = {0};  

MPU6050 mpu;
BleMouse bleMouse("ESP32 Air Mouse", "Espressif", 100);  // BLE HID Mouse

// Define threshold for flex sensor to trigger left-click
#define FLEX_THRESHOLD 550 // Adjust this value based on your flex sensor's output
#define MIN_FLEX_VALUE 15  // Minimum flex value to avoid zero voltage (adjust as needed)

int movingAverage(int newVal, int history[], int size) {
    history[filterIndex] = newVal;
    filterIndex = (filterIndex + 1) % size;

    int sum = 0;
    for (int i = 0; i < size; i++) sum += history[i];
    
    return sum / size;  
}

void setup() {
    Serial.begin(115200);
    Wire.begin();

    if (!bleMouse.isConnected()) {
    Serial.println("Reconnecting BLE...");
    bleMouse.begin();
}


    // Initialize MPU6050
    mpu.initialize();
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed!");
        while (1);
    }
    Serial.println("MPU6050 Connected!");

    // Start BLE Mouse
    Serial.println("Starting BLE...");
    bleMouse.begin();
    Serial.println("BLE Started");

    // Set up button as input with pull-up resistor
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Set up flex sensor as input (analog pin)
    pinMode(FLEX_PIN, INPUT);
}

void loop() {
    if (bleMouse.isConnected()) {
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        
        //Scrolling Function
        if (digitalRead(SCROLL_BUTTON_PIN) == LOW) {  // Button pressed
            int scrollDirection = 0;

            if (gy > 1000) {  // Tilt forward (scroll down)
                scrollDirection = -1;
            } else if (gy < -1000) {  // Tilt backward (scroll up)
                scrollDirection = 1;
            }

            if (scrollDirection != 0) {
                bleMouse.move(0, 0, scrollDirection);
                Serial.printf("Scrolling: %s\n", scrollDirection == 1 ? "Up" : "Down");
            }
        }

        delay(50);  // Adjust delay for responsiveness
        // Convert gyro values to mouse movement
        int xMove = gx / 1000;  // Scale gyroscope values
        int yMove = gz / 1000;  // Invert Y-axis for natural control

        // Apply moving average filter
        xMove = movingAverage(xMove, xHistory, FILTER_SIZE);
        yMove = movingAverage(yMove, yHistory, FILTER_SIZE);

        // Apply dead zone to prevent jittering
        if (abs(xMove) < 2) xMove = 0;
        if (abs(yMove) < 2) yMove = 0;

        // Move the mouse
        bleMouse.move(xMove, -yMove);
        Serial.printf("Mouse Move: X=%d, Y=%d\n", xMove, yMove);

        // Check button state for right-click
        if (digitalRead(BUTTON_PIN) == LOW) { // Button is pressed
            Serial.println("Right Click!");
            bleMouse.click(MOUSE_RIGHT);
            delay(500); // Debounce delay to avoid multiple clicks
        }

        // Read the flex sensor value
        int flexValue = analogRead(FLEX_PIN);

        // Check if the flex sensor value exceeds the threshold to trigger left-click
        // Also ensure that flex value is above the MIN_FLEX_VALUE to avoid zero voltage
        if (flexValue < FLEX_THRESHOLD && flexValue > MIN_FLEX_VALUE) {
            Serial.println("Left Click!");
            bleMouse.click(MOUSE_LEFT);
            delay(500); // Debounce delay to avoid multiple clicks
        }

        delay(10);  // Adjust delay for smoothness
    }
}
