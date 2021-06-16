[![CircleCI](https://circleci.com/gh/Vladislavo/pills_counter.svg?style=shield)](https://circleci.com/gh/Vladislavo/pills_counter)
[![Code Quality Score](https://www.code-inspector.com/project/23693/score/svg)](https://www.code-inspector.com/public/project/23693/pills_counter/dashboard)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

# Pills Counter

Applying Computer Vision and Machine Learning (ML) at the very edge of embedded development. This project uses [TTGO T-CAMERA](https://www.electronics-lab.com/ttgo-t-camera-esp32-cam-board-oled-ai-capabilities/ "TTGO T-CAMERA") board based on ESP32 MCU for counting pills applying various ML techniques, such as Support Vector Machine (SVM) and Random Forest. Pills are just an example of what can be counted. The project has a modular structure and other classes and categories can be easily added.

![Pills Counter Demo](demo/counter_demo.gif)

HTML monitor and configuration interface is provided. Counter enabling, mode, reset, and alarm can be set up. Alarms work sending a JSON message to an indicated HTTP endpoint when the alarm is enabled and introduced counter value has been reached.

The device is intended to be installed over a production line/conveyor such as the whole belt width is captured by the camera sensor.

## Counting Methods

During the development process various counting strategies have been elaborated.

1. __Grid Division__. Bruit force approach that splits the image in sample-sized subframes and applies a classifier to each. Does not support accumulative couning due to inefficiency of pills identification strategies. 
2. __One Detection Line__ and __One Detection Line with 1 Frame Hysteresis__. More elegant, robust, and efficient way of counting pills. It consists of placing a detection line throughout the image width. The detection line has a hight of subframe and a width of image. Numerious overlapped subframes are placed over the detection line so that high pill detection precision is achieved.
3. __Two Detection Lines__. This method is based on placing two detection lines over the image. The idea is to capture pills on the first line and await their confirmation plus uncaptured ones on the second line. This method requires calibration due to varying production belt speed. The distance between two lines is a function of the belt speed.

## Classification Algorithms

Once a subframe has been extracted, a classifier should be applied to detect a pill. Next algorithms has been used for classification:

- SVM
- Random Forest (RF)

The ML models data, training, and porting code is [here](https://github.com/Vladislavo/pills_counter_ml_model_training "here"). Refer to Credits section for used projects.

## Results

All initial goals of the project have been met and the results are presented here as throughput and algorithms efficiency.

### Throughput

The device works in two modes: HTTP monitored and Unattended. For the HTTP monitored mode the average is __4.2 fps__, and for the Unattended mode is __8.3fps__. Throughput depends on the operations listed in the table below. Memory access times are included in operations.

| Operation | Average Time, ms | Mode | 
|-----|-----|-----|
| JPG -> RGB888 | 97 | Monitored and Unattended |
| Classification Processing | 3 (RF), 989 (SVM) | Monitored and Unattended |
| RGB888 -> JPG | 105 | Monitored only |

### Algorithms Efficiency

| Algorithm | Average Classification Time, ms | 
|-----|-----|
| SVM | 52 |
| RF (forest depth 20) | 0.16 |
| RF (forest depth 30) | 0.2 |
| RF (forest depth 40) | 0.3 |

## Potential Future Improvements

- More classification algorithms
- More classification categories
- Section on ML model validation
- Use of color images

## Installation

Development has been done using Visual Studio Code and PlatformIO extention. To install download VS Code, install PlatformIO, compile and program your device.

## Credits

Thanks to Espressif for the [IDF](https://idf.espressif.com/ "Espressif IDF"). Important credits should be given to two projects used for ML model training and porting. For the training [scilearn-kit](https://scikit-learn.org/stable/index.html "scilearn-kit") project has been used, and thanks to [eloquentarduino](https://github.com/eloquentarduino "eloquentarduino") and his [micromlgen](https://github.com/eloquentarduino/micromlgen "micromlgen") library the porting has been possible.

## License

The source code for the project is licensed under the MIT license, which you can find in the LICENSE.txt file.