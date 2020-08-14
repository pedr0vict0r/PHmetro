EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L arduino:Arduino_Mega2560_Shield XA?
U 1 1 5BEAE515
P 3100 4350
F 0 "XA?" H 3100 1970 60  0000 C CNN
F 1 "Arduino_Mega2560_Shield" H 3100 1864 60  0000 C CNN
F 2 "" H 3800 7100 60  0001 C CNN
F 3 "https://store.arduino.cc/arduino-mega-2560-rev3" H 3800 7100 60  0001 C CNN
	1    3100 4350
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J1
U 1 1 5BEAE61E
P 6150 1700
F 0 "J1" H 6230 1692 50  0000 L CNN
F 1 "Bluetooth" H 6230 1601 50  0000 L CNN
F 2 "" H 6150 1700 50  0001 C CNN
F 3 "~" H 6150 1700 50  0001 C CNN
	1    6150 1700
	1    0    0    -1  
$EndComp
Text GLabel 5700 1600 0    50   Input ~ 0
5V
Wire Wire Line
	5950 1600 5700 1600
Text GLabel 1500 6200 0    50   Input ~ 0
5V
Wire Wire Line
	1800 6200 1500 6200
$EndSCHEMATC
