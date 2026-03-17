#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneButton.h>
#include <ESP32Servo.h>
#include <Math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

const int pushbuttonRed = 15;
const int pushbuttonBlue = 16;
const int trigPin = 10;
const int echoPin = 7;
const int SDAPin = 4;
const int SCLPin = 5;
const int servoPin = 6;
const int buzzerPin = 13;
const int potPin = 17;

Servo myservo;
OneButton buttonRed(pushbuttonRed, true);
OneButton buttonBlue(pushbuttonBlue, true);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int centerX = SCREEN_WIDTH / 2;
const int centerY = SCREEN_HEIGHT - 1;
const int maxRadius = 50;
const int maxDistance = 300;

struct Blip {
    float angle;
    int distance;
    unsigned long timestamp;
    bool active;
};

#define MAX_BALLS 3

struct Ball {
    bool active;
    float x, y;
    float vx, vy;
    int birthDistance;
};

Blip blips[20];
int blipscount = 0;
unsigned long lastUpdate = 0;
const int fadeTime = 3000;

Ball balls[MAX_BALLS];
int ballcount = 0;

void setup() {
    Serial.begin(115200);
    pinMode(pushbuttonRed, INPUT_PULLUP);
    pinMode(pushbuttonBlue, INPUT_PULLUP);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(potPin, INPUT);
    
    myservo.attach(servoPin);
    myservo.write(0);
    
    Wire.begin(SDAPin, SCLPin);
    
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    
    display.clearDisplay();
    display.display();
    
    for(int i = 0; i < 20; i++){
        blips[i].active = false;
    }
    
    for(int i = 0; i < MAX_BALLS; i++){
        balls[i].active = false;
    }
}

bool redbuttonState(){
    int buttonvalNew;
    buttonvalNew = digitalRead(pushbuttonRed);
    static int buttonvalOld = 0;
    static bool redValstate = true;
    
    if(buttonvalNew == 0){
        tone(buzzerPin, 2000);
    } else if(buttonvalNew == 1){
        noTone(buzzerPin);
    }
    
    if(buttonvalOld == 0 && buttonvalNew == 1){
        redValstate = !redValstate;
    }
    
    buttonvalOld = buttonvalNew;
    return redValstate;
}

bool bluebuttonState(){
    int buttonvalNew;
    static bool blueValstate = true;
    static int buttonvalOld = 0;
    buttonvalNew = digitalRead(pushbuttonBlue);
    
    if(buttonvalNew == 0){
        tone(buzzerPin, 2000);
    } else if(buttonvalNew == 1){
        noTone(buzzerPin);
    }
    
    if(buttonvalOld == 0 && buttonvalNew == 1){
        blueValstate = !blueValstate;
    }
    
    buttonvalOld = buttonvalNew;
    return blueValstate;
}

float measureDistance(){
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    float duration = pulseIn(echoPin, HIGH, 30000);
    if(duration == 0) return maxDistance;
    
    long distance = (0.0343 * duration) / 2;
    return (distance > maxDistance) ? maxDistance : distance;
}

void addBlip(float angle, int distance){
    if(blipscount < 20 && distance < maxDistance){
        blips[blipscount].distance = distance;
        blips[blipscount].angle = angle;
        blips[blipscount].timestamp = millis();
        blips[blipscount].active = true;
        blipscount++;
    }
}

void updateBlips(){
    unsigned long currentTime = millis();
    for(int i = 0; i < blipscount; i++){
        if(blips[i].active && (currentTime - blips[i].timestamp) > fadeTime){
            blips[i].active = false;
            
            for(int j = i; j < blipscount - 1; j++){
                blips[j] = blips[j + 1];
            }
            blipscount--;
            i--;
        }
    }
}

void drawBlip(){
    unsigned long currentTime = millis();
    for(int i = 0; i < blipscount; i++){
        if(blips[i].active){
            float age = (float)(currentTime - blips[i].timestamp) / fadeTime;
            int x = centerX + (blips[i].distance * maxRadius / maxDistance) * cos(radians(blips[i].angle));
            int y = centerY - (blips[i].distance * maxRadius / maxDistance) * sin(radians(blips[i].angle));
            
            if(age < 0.5){
                display.drawLine(x-2, y-2, x+2, y+2, SSD1306_WHITE);
                display.drawLine(x-2, y+2, x+2, y-2, SSD1306_WHITE);
            } else {
                display.drawLine(x-2, y-2, x+2, y+2, SSD1306_WHITE);
                display.drawLine(x-2, y+2, x+2, y-2, SSD1306_WHITE);
            }
        }
    }
}

void drawRadar(int centerX, int centerY, int maxRadius){
    int startAngle = 0;
    int endAngle = 180;
    
    for(int r = maxRadius; r >= maxRadius/2; r -= maxRadius/2){
        for(int angle = startAngle; angle <= endAngle; angle++){
            int x1 = centerX + r * cos(radians(angle));
            int y1 = centerY - r * sin(radians(angle));
            int x2 = centerX + r * cos(radians(angle+1));
            int y2 = centerY - r * sin(radians(angle+1));
            display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
        }
    }
}

void addBalls(int distance){
    if(ballcount == MAX_BALLS){
        for(int i = 0; i < MAX_BALLS - 1; i++){
            balls[i] = balls[i + 1];
        }
        ballcount--;
    }
    
    Ball &b = balls[ballcount];
    b.active = true;
    b.x = random(10, 118);
    b.y = random(10, 50);
    
    float speed = map(distance, 0, 400, 16, 6);
    b.vx = random(4, 12);
    if(b.vx == 0) b.vx = 4;
    b.vy = speed;
    
    b.birthDistance = distance;
    ballcount++;
}

void updateBalls(){
    const int r = 3;
    for(int i = 0; i < ballcount; i++){
        if(!balls[i].active) continue;
        Ball &b = balls[i];
        b.x += b.vx;
        b.y += b.vy;
        
        if(b.x <= r || b.x >= 127 - r) b.vx = -b.vx;
        if(b.y <= r || b.y >= 60 - r) b.vy = -b.vy;
    }
}

void drawbouncingBall(){
    const int r = 3;
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    
    for(int i = 0; i < ballcount; i++){
        if(!balls[i].active) continue;
        Ball &b = balls[i];
        
        b.x = constrain(b.x, r, 127 - r);
        b.y = constrain(b.y, r, 60 - r);
        
        display.fillCircle((int)b.x, (int)b.y, r, SSD1306_WHITE);
        
        int boxX = constrain((int)b.x - 10, 0, 108);
        int boxY = 58;
        display.fillRoundRect(boxX, boxY, 20, 4, 1, SSD1306_WHITE);
    }
}

void loop(){
    static int angle = 0;
    static bool forward = true;
    static unsigned long lastServoMove = 0;
    static unsigned long lastMeasurement = 0;
    
    bool redValstate = redbuttonState();
    bool blueValstate = bluebuttonState();
    
    if(redValstate == 1 && blueValstate == 1){
        unsigned long currentTime = millis();
        if(currentTime - lastServoMove >= 5){
            myservo.write(angle);
            if(forward){
                angle += 2;
                if(angle >= 180){
                    angle = 180;
                    forward = false;
                }
            } else {
                angle -= 2;
                if(angle <= 0){
                    angle = 0;
                    forward = true;
                }
            }
            lastServoMove = currentTime;
        }
        
        if(currentTime - lastMeasurement >= 100){
            float distance = measureDistance();
            if(distance < maxDistance){
                addBlip(angle, (int)distance);
            }
            lastMeasurement = currentTime;
        }
        
        if(currentTime - lastUpdate >= 33){
            display.clearDisplay();
            updateBlips();
            drawRadar(centerX, centerY, maxRadius);
            
            int sweepX = centerX + maxRadius * cos(radians(angle));
            int sweepY = centerY - maxRadius * sin(radians(angle));
            display.drawLine(centerX, centerY, sweepX, sweepY, SSD1306_WHITE);
            
            drawBlip();
            display.display();
            lastUpdate = currentTime;
        }
    } else if(redValstate == 1 && blueValstate == 0){
        unsigned long currentTime = millis();
        int potval = analogRead(potPin);
        angle = map(potval, 0, 4095, 0, 180);
        myservo.write(angle);
        
        if(currentTime - lastMeasurement >= 100){
            float distance = measureDistance();
            if(distance < maxDistance){
                addBlip(angle, (int)distance);
            }
            lastMeasurement = currentTime;
        }
        
        if(currentTime - lastUpdate >= 33){
            display.clearDisplay();
            updateBlips();
            drawRadar(centerX, centerY, maxRadius);
            
            int sweepX = centerX + maxRadius * cos(radians(angle));
            int sweepY = centerY - maxRadius * sin(radians(angle));
            display.drawLine(centerX, centerY, sweepX, sweepY, SSD1306_WHITE);
            
            drawBlip();
            display.display();
            lastUpdate = currentTime;
        }
    } else if(redValstate == 0 && blueValstate == 1){
        static int distanceOld = -1;
        int distanceNew = measureDistance();
        
        if(abs(distanceNew - distanceOld) > 5){
            addBalls(distanceNew);
            distanceOld = distanceNew;
        }
        
        display.clearDisplay();
        updateBalls();
        drawbouncingBall();
        display.display();
    }
}


