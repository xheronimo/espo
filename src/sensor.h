// src/sensor.h - Archivo de definiciones de sensores
#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

// Funciones para lectura de sensores
bool leerSensorNivel(float *nivel);
float leerSensorAnalogico(int canal);
bool leerMedidorPotencia(float *tensiones, float *corrientes);
bool leerSHT31(float *temperatura, float *humedad);
bool leerEstadoMotor(int motor);

// Funciones de control
bool arrancarMotor(int motor, float velocidad);
bool pararMotor(int motor);
bool alternarMotores();
bool controlAutomaticoMotores(void *data);

#endif
