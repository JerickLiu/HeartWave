# HeartWave

# Overview

HeartWave is a software-based prototype of a Heart Rate Variability (HRV) device developed using **C++** and the **Qt** framework. The goal of HeartWave is to measure, analyze, and display HRV patterns in real-time, providing users with biofeedback on their coherence levels. The device aims to help users reduce stress and achieve a high state of coherence, an optimal psychophysiological state that reflects well-being and health.

![](https://github.com/JerickLiu/HeartWave/blob/main/HeartWave_Demo.gif)

# Features

- A light on the machine and a symbol on the screen that indicates an active pulse reading.
- A user interface consisting of a screen and buttons, including an off/on button for the device, a menu button, a standard back button, directional arrow buttons, and a selector in the center of the arrow buttons which also functions as a start/stop button in session mode.
- A light that changes to red, blue, or green indicating low, medium, or high coherence, depending on the challenge level.
- A session screen that displays the main HRV graph (HR vs time) with key metrics.
- A breath pacer designed to guide the user's breathing to help achieve a high state of coherence.
- A settings tab that includes challenge level and breath pacer settings.
- A log and history tab of all sessions, with dates, when selected show the summary view, as well as the ability to delete a session.
- A built-in database system to maintain data persistence.
- An option to reset, wipe all data and restore the device to the initial install condition.
- A battery charge indicator on the session screen.

# How to Run
1) Clone the repository
2) Open the project in QT Creator, selecting the HeartWave.pro file
3) Build and run the project in QT Creator, using the play button in the bottom left
