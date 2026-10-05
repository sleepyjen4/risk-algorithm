#include <stdio.h>
#include <stdbool.h>

/* ===== Tuning settings: feel free to modify lol ===== */

/* Temperature (°C) */
const float TEMP_SAFE = 22.0f;
const float TEMP_DANGER = 30.0f;

/* Turbidity (NTU) */
const float TURBIDITY_SAFE = 80.0f;
const float TURBIDITY_DANGER = 240.0f;

/* pH */
const float PH_HIGH_SAFE = 8.5f;
const float PH_HIGH_DANGER = 9.5f;
const float PH_LOW_SAFE = 6.5f;
const float PH_LOW_DANGER = 5.5f;

/* Weights (all add up to 1) */
const float TEMP_WEIGHT = 0.45f;
const float TURBIDITY_WEIGHT = 0.3f;
const float PH_WEIGHT = 0.25f;

/* Smoothing */
const float SMOOTHING = 0.2f;

/* Pump switching (risk out of 100) */
const float PUMP_ON = 60.0f;
const float PUMP_OFF = 45.0f;

/* Valid sensor ranges: readings outside means the sensor is broken */
const float TEMP_MIN = 0.0f;
const float TEMP_MAX = 45.0f;
const float TURBIDITY_MIN = 0.0f;
const float TURBIDITY_MAX = 3000.0f;
const float PH_MIN = 2.0f;
const float PH_MAX = 12.0f;


/* ===== Program state: don't edit unless needed plez ;-; ===== */

float smoothedRisk = -1.0f;
bool pumpOn = false;

/*
 * ramp: turns a sensor reading into a worry score from 0 to 1.
 *   value  = the sensor reading
 *   safe   = worry starts here
 *   danger = worry is maxed out here
 */
float ramp(float value, float safe, float danger) {
    if (value <= safe) {
        return 0.0f;
    }
    if (value >= danger) {
        return 1.0f;
    }
    return (value - safe) / (danger - safe);
}

/*
 * rampLow: similar to ramp, but worry rises as the reading drops.
 *   value = the sensor reading
 *   safe   = worry starts when the reading falls below this
 *   danger = worry is maxed out at or below this
 */
float rampLow(float value, float safe, float danger) {
    if (value >= safe) {
        return 0.0f;
    }
    if (value <= danger) {
        return 1.0f;
    }
    return (safe - value) / (safe - danger);
}

/*
 * temperatureRamp: turns a temperature reading into a worry score from 0 to 1.
 *   temperature = water temperature in °C
 */
float temperatureRamp(float temperature) {
    return ramp(temperature, TEMP_SAFE, TEMP_DANGER);
}

/*
 * turbidityRamp: turns a turbidity reading into a worry score from 0 to 1.
 *   turbidity = turbidity in NTU
 */
float turbidityRamp(float turbidity) {
    return ramp(turbidity, TURBIDITY_SAFE, TURBIDITY_DANGER);
}

/*
 * phRamp: turns a pH reading into a worry score from 0 to 1.
 *   ph = pH reading (healthy between PH_LOW_SAFE and PH_HIGH_SAFE)
 */
float phRamp(float ph) {
    float highWorry = ramp(ph, PH_HIGH_SAFE, PH_HIGH_DANGER);
    float lowWorry = rampLow(ph, PH_LOW_SAFE, PH_LOW_DANGER);
    return highWorry + lowWorry;
}

/*
 * riskScore: combines all three sensor readings into one risk from 0 to 100.
 *   temperature = water temperature in °C
 *   turbidity   = turbidity in NTU
 *   ph          = pH reading
 */
float riskScore(float temperature, float turbidity, float ph) {
    float combined = (temperatureRamp(temperature) * TEMP_WEIGHT) + (turbidityRamp(turbidity) * TURBIDITY_WEIGHT) + (phRamp(ph) * PH_WEIGHT);
    return combined * 100.0f;
}

/*
 * smoothRisk: blends a new risk score into the running smoothed risk.
 *   newRisk = the latest risk from riskScore (0 to 100)
 */
float smoothRisk(float newRisk) {
    if (smoothedRisk < 0.0f) {
        smoothedRisk = newRisk;
    } else {
        smoothedRisk = (smoothedRisk * (1.0f - SMOOTHING)) + (newRisk * SMOOTHING);
    }
    return smoothedRisk;
}

/*
 * updatePump: decides whether the aeration pump should be on.
 *   risk = the smoothed risk (0 to 100)
 */
bool updatePump(float risk){
    if (risk >= PUMP_ON) {
        pumpOn = true;
    } else if (risk <= PUMP_OFF && pumpOn) {
        pumpOn = false;
    }
    return pumpOn;
}

/*
 * isValidReading: checks a set of readings could be real, not a broken sensor.
 *   temperature = water temperature in °C
 *   turbidity   = turbidity in NTU
 *   ph          = pH reading
 */
bool isValidReading(float temperature, float turbidity, float ph) {
    if (temperature <= TEMP_MIN || temperature > TEMP_MAX) {
        return false;
    }
    if (turbidity <= TURBIDITY_MIN || turbidity > TURBIDITY_MAX) {
        return false;
    }
    if (ph < PH_MIN || ph > PH_MAX) {
        return false;
    }
    return true;
}

/*
 * processReading: runs one set of sensor readings through the whole risk algorithm.
 *   temperature = water temperature in °C
 *   turbidity   = turbidity in NTU
 *   ph          = pH reading
 */
void processReading(float temperature, float turbidity, float ph) {
    if (!isValidReading(temperature, turbidity, ph)) {
        printf("Invalid reading.\n");
        return;
    }
    float rawRisk = riskScore(temperature, turbidity, ph);
    float smooth = smoothRisk(rawRisk);
    bool pump = updatePump(smooth);
    printf("Risk: %.1f  Pump: %d\n", smooth, pump);
}


// Random Test :)
int main(void) {
    printf("Calm morning:\n");
    processReading(20.0f, 90.0f, 7.5f);

    printf("Sensor glitch:\n");
    processReading(0.0f, 90.0f, 7.5f);

    printf("Heatwave and algal bloom:\n");
    for (int i = 0; i < 8; i++) {
        processReading(31.0f, 230.0f, 9.3f);
    }

    printf("Cool change:\n");
    for (int i = 0; i < 8; i++) {
        processReading(20.0f, 90.0f, 7.5f);
    }

    return 0;
}
