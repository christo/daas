#!/bin/bash

#desk control
# usage: d <command> 
# where <command is a daas command>
arg=${1:-"s"}
export ARDUINO_PORT=/dev/cu.usbmodem*
stty -f $ARDUINO_PORT cread cs8
echo $arg > $ARDUINO_PORT

