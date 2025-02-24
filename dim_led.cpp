int ledPin = 8; // LED connected to digital pin 10
void setup()
{
    // declaring LED pin as output
    pinMode(ledPin, OUTPUT);
}
void loop()
{
    // fade in from min to max in increments of 5 points:
    for (int fadeValue = 0; fadeValue <= 255; fadeValue += 5)
    {
        // sets the value (range from 0 to 255):
        analogWrite(ledPin, fadeValue);
        // wait for 30 milliseconds to see the dimming effect
        delay(30);
    }
    // fade out from max to min in increments of 5 points:
    for (int fadeValue = 255; fadeValue >= 0; fadeValue -= 5)
    {
        // sets the value (range from 0 to 255):
        analogWrite(ledPin, fadeValue);
        // wait for 30 milliseconds to see the dimming effect
        delay(30);
    }
}

const int sensorPin = A5;
const int ledPin = 8;
void setup()
{
    pinMode(sensorPin, INPUT); // declare the sensorPin as an INPUT
    pinMode(ledPin, OUTPUT);   // declare the ledPin as an OUTPUT
}
void loop()
{
    // read the value from the sensor:
    int sensorValue = analogRead(sensorPin);
    // turn the ledPin on
    digitalWrite(ledPin, HIGH);
    // stop the program for <sensorValue> milliseconds:
    delay(sensorValue);
    // turn the ledPin off:
    digitalWrite(ledPin, LOW);
    // stop the program for for <sensorValue> milliseconds:
    delay(sensorValue);
}
