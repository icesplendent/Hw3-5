#include "accelerometer.h"
#include "gyro.h"
#include "mbed.h"
using namespace std::chrono_literals;
static BufferedSerial pc_uart(USBTX, USBRX);  // define USB UART port to PC

#define waveformLength (128)
#define lookUpTableDelay (10)
#define bufferLength (32)

AnalogOut Aout(PA_4);

InterruptIn keyboard0(D2);
InterruptIn keyboard1(D3);
InterruptIn keyboard2(D4);
InterruptIn button(BUTTON1);

EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;
int idC = 0;
int idE = 0;
int idG = 0;

Accelerometer acc;
Gyro gyro;
double Accel[3] = {0};
double Gyro[3] = {0};
double accAngleX = 0;
double accAngleY = 0;
double elapsedTime = 0;
double roll, pitch, yaw;
double gyroAngleX = 0;
double gyroAngleY = 0;
int counter = 0;
int idR[32] = {0};
int indexR = 0;
double temp[3] = {0};
int during = 8;

float waveform[waveformLength] = {  // 128 samples of a sine waveform
    0.500, 0.525, 0.549, 0.574, 0.598, 0.622, 0.646, 0.670, 0.693, 0.715, 0.737,
    0.759, 0.780, 0.800, 0.819, 0.838, 0.856, 0.873, 0.889, 0.904, 0.918, 0.931,
    0.943, 0.954, 0.964, 0.972, 0.980, 0.986, 0.991, 0.995, 0.998, 1.000, 1.000,
    0.999, 0.997, 0.994, 0.989, 0.983, 0.976, 0.968, 0.959, 0.949, 0.937, 0.925,
    0.911, 0.896, 0.881, 0.864, 0.847, 0.829, 0.810, 0.790, 0.769, 0.748, 0.726,
    0.704, 0.681, 0.658, 0.634, 0.610, 0.586, 0.562, 0.537, 0.512, 0.488, 0.463,
    0.438, 0.414, 0.390, 0.366, 0.342, 0.319, 0.296, 0.274, 0.252, 0.231, 0.210,
    0.190, 0.171, 0.153, 0.136, 0.119, 0.104, 0.089, 0.075, 0.063, 0.051, 0.041,
    0.032, 0.024, 0.017, 0.011, 0.006, 0.003, 0.001, 0.000, 0.000, 0.002, 0.005,
    0.009, 0.014, 0.020, 0.028, 0.036, 0.046, 0.057, 0.069, 0.082, 0.096, 0.111,
    0.127, 0.144, 0.162, 0.181, 0.200, 0.220, 0.241, 0.263, 0.285, 0.307, 0.330,
    0.354, 0.378, 0.402, 0.426, 0.451, 0.475, 0.500};

char serialInBuffer[bufferLength];
int serialCount = 0;

void loadWaveform(void) {
    char byteIn;
    int i = 0;
    serialCount = 0;
    Aout = 0;
    printf("Loading Waveform ...\r\n");
    while (i < waveformLength) {
        if (pc_uart.readable()) {
            pc_uart.read(&byteIn, 1);  // read one char
            serialInBuffer[serialCount] = byteIn;
            serialCount++;
            if (serialCount == 5) {
                serialInBuffer[serialCount] = '\0';
                waveform[i] = (float)atof(serialInBuffer);
                serialCount = 0;
                i++;
            }
        }
    }
    printf("Waveform Loaded\r\n");
}

void printWaveform(void) {
    serialCount = 0;

    printf("begin\n");
    for (int i = 0; i < waveformLength; i++) {
        printf("%f\t", waveform[i]);
        if (i > 0 && i % 10 == 0) printf("\n");
        // printf("\n");
    }
    printf("\n");
    printf("end\n");
}

void playNote(int freq, int duration) {
    int i = duration * freq / during;
    int j = waveformLength;
    // 1000000 us = 1s, 1 period need to show freq 次 wave, each wavefrom is
    // contrcuted by 128 samples (sine wave),
    int waitTime = (1000000 / waveformLength / freq - lookUpTableDelay) << 0;
    printf("Play notes %d %d %f\n", freq, during, pitch + roll + yaw);
    while (i--) {
        j = waveformLength;
        while (j--) {
            Aout = waveform[j];
            wait_us(waitTime);
        }
    }
}

void loadWaveformHandler(void) { queue.call(loadWaveform); }

// Play a note at default duration of 1s
void playNoteC(void) {
    idC = queue.call_every(1ms, playNote, 131, 1);
}  // freq = 131hz (Do), duration = 1
void playNoteE(void) { idE = queue.call_every(1ms, playNote, 165, 1); }
void playNoteG(void) { idG = queue.call_every(1ms, playNote, 196, 1); }

void stopPlayNoteC(void) { queue.cancel(idC); }
void stopPlayNoteE(void) { queue.cancel(idE); }
void stopPlayNoteG(void) { queue.cancel(idG); }

void record(void) {
    temp[0] = 0;
    temp[1] = 0;
    temp[2] = 0;

    // 10 samples to average
    for (int i = 0; i < 10; i++) {
        acc.GetAcceleromterSensor(Accel);
        acc.GetAcceleromterCalibratedData(Accel);
        temp[0] += Accel[0];
        temp[1] += Accel[1];
        temp[2] += Accel[2];
    }

    Accel[0] = temp[0] / 10;
    Accel[1] = temp[1] / 10;
    Accel[2] = temp[2] / 10;

    // printf("Calibrated ACC= %f, %f, %f\n", Accel[0], Accel[1], Accel[2]);

    // Calculating Roll and Pitch from the accelerometer data
    accAngleX =
        (atan(Accel[1] / sqrt(Accel[0] * Accel[1] + Accel[2] * Accel[2])) *
         180 / SENSOR_PI_DOUBLE);
    accAngleY =
        (atan(-1 * Accel[1] / sqrt(Accel[1] * Accel[1] + Accel[2] * Accel[2])) *
         180 / SENSOR_PI_DOUBLE);

    gyro.GetGyroSensor(Gyro);
    gyro.GetGyroCalibratedData(Gyro);

    // printf("Calibrated Gyro= %f, %f, %f\n", Gyro[0], Gyro[1], Gyro[2]);
    elapsedTime = 0.1;  // 100ms by thread sleep time
    // Currently the raw values are in degrees per seconds, deg/s, so we need to
    // multiply by sendonds (s) to get the angle in degrees
    gyroAngleX = gyroAngleX + Gyro[0] * elapsedTime;  // deg/s * s = deg
    gyroAngleY = gyroAngleY + Gyro[1] * elapsedTime;
    yaw = yaw + Gyro[2] * elapsedTime;
    // Complementary filter - combine acceleromter and gyro angle values
    // roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
    // pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
    // Use Acc data only
    roll = accAngleX;
    pitch = accAngleY;
    printf("%f/%f/%f\n", roll, pitch, yaw);

    if (yaw + roll + pitch <= 0) {
        during = 8;
    } else if (yaw + roll + pitch <= 2) {
        during = 4;
    } else if (yaw + roll + pitch <= 4) {
        during = 2;
    } else {
        during = 1;
    }

    // ThisThread::sleep_for(10ms);
}

// modify: record every 10ms, and average the results at 100ms (10 samples)
void startRecord(void) {
    // printf("---start---\n");
    idR[indexR++] = queue.call_every(10ms, record);
    indexR = indexR % 32;
}

void stopRecord(void) {
    // printf("---stop---\n");
    for (auto &i : idR) queue.cancel(i);
}

int main(void) {
    t.start(callback(&queue, &EventQueue::dispatch_forever));
    //   button.rise(queue.event(loadWaveformHandler));
    button.fall(queue.event(startRecord));
    button.rise(queue.event(stopRecord));
    keyboard0.fall(queue.event(playNoteC));
    keyboard1.fall(queue.event(playNoteE));
    keyboard2.fall(queue.event(playNoteG));
    keyboard0.rise(queue.event(stopPlayNoteC));
    keyboard1.rise(queue.event(stopPlayNoteE));
    keyboard2.rise(queue.event(stopPlayNoteG));
    //   loadWaveform(); //Comment out to test the keyboard
    printWaveform();
}
