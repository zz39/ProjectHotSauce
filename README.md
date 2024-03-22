# Project 'Hot Sauce'
## Description

This project implements a simple IoT device using an ESP 8266 Wifi, SHT31-D sensor for temperature and humidity monitoring, PMS5003 sensor for AQI monitoring, and S8-0053 sensor for CO2 monitoring. The device sends the sensor data to an AWS Lambda function through HTTP POST requests, which then stores the data in an AWS DynamoDB database. The sensor module is powered by a USB connection and can be easily deployed for continuous monitoring in indoor environments.

## Features

- Real-time monitoring of temperature, humidity, AQI, and CO2 levels
- Automatic data logging to an AWS DynamoDB database
- Simple and low-cost hardware setup
- Easily scalable for multiple sensor nodes

## Setup

### Hardware Requirements

- ESP 8266 Micro-Controller
- SHT31-D sensor
- PMS5003 sensor
- S8-0053 sensor
- USB cable for power

### Software Requirements

- Arduino IDE
- AWS account with Lambda and DynamoDB set up
- WiFi network for connectivity

### Usage

***TBD***

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
