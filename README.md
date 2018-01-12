# MiningSpeedChecker

A tool for Windows and Linux I created in an evening to check on my hush.miningspeed.com miner by interacting with the miningspeed API using libcurl, rapidJSON to parse, and FLTK as the GUI. I thought I would share it.

## Windows
There is an executable I compiled in the Release section.

## Build Ubuntu
```sh
sudo apt update
sudo apt install build-essential cmake git libcurl4-openssl-dev rapidjson-dev libfltk1.3-dev
git clone https://github.com/anhydrous99/MiningSpeedChecker
cd MiningSpeedChecker
mkdir build && cd build
cmake ..
make
```
