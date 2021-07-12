nRF9160: GPS Cold Start Testing (Modified Example)
##################################################

.. contents::
   :local:
   :depth: 2

Overview and Motivation
***********************

This example has been modified from the "gps" example to emulate and facilitate cold start testing.
Rather than waiting for GPS data to become invalid over time, this example will clear GPS data in the modem before attempting another fix.
Once a fix is achieved, a new fix can be attempted through a button press.
This example can work either with or without SUPL. 

Changes from Default Example
****************************

* Enables modem trace
* Measures TTFF (Time To First Fix)
* Adds button 1 support to attempt another fix
* Clears all GPS data (except TCXO offset) in the modem prior to attempting a fix
* Print injected location from SUPL

The default "gps" example was used as the base for this example so you can see the changes from the example by going through the commits.

Building and running
********************

This example should be built similarly as mentioned in the "gps" example.

Usage
=====

Upon boot up, it will start to look for a fix and continue until a fix is achieved.
After a fix, it will prompt the user to press button 1 to attempt another fix.


Sample output (without A-GPS)
=============================

When booting up, you should see the following printout.

.. code-block:: console

   *** Booting Zephyr OS build v2.4.99-ncs1  ***
   Starting GPS application
   GPS Socket created
   Getting GPS data...
   Tracking: 0 Using: 0 Unhealthy: 0
   ---------------------------------
   Seconds since last fix: 0
   Searching [/]

   NMEA strings:

   ---------------------------------

Several seconds later:

.. code-block:: console

   Tracking: 7 Using: 0 Unhealthy: 0
   ---------------------------------
   Seconds since last fix: 10
   Searching [/]

   NMEA strings:

   $GPGGA,000010.43,,,,,0,,99.99,,M,,M,,*60
   $GPGLL,,,,,000010.43,V,N*4C
   $GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,1*2D
   $GPGSV,2,1,7,2,,,39,5,,,44,12,,,43,18,,,40,1*54
   $GPGSV,2,2,7,20,,,10,25,,,45,29,,,43,1*5A
   $GPRMC,000010.43,V,,,,,,,060180,,,N,V*0E
   ---------------------------------
   
Once the fix is realized (typically 25 - 40 seconds in good conditions):

.. code-block:: console

   Tracking: 12 Using: 5 Unhealthy: 0
   ---------------------------------
   Longitude:  -121.999802
   Latitude:   37.321312
   Altitude:   39.332546
   Speed:      0.273315
   Heading:    0.000000
   Date:       2021-07-08
   Time (UTC): 19:35:30
   Fix location: 37.32131157, -121.99980177

   Time to get fix: 27 seconds
   ---------------------------------
   NMEA strings for GPS fix:

   $GPGGA,193530.37,3719.27869,N,12159.98811,W,1,05,1.93,39.33,M,,M,,*54
   $GPGLL,3719.27869,N,12159.98811,W,193530.37,A,A*7B
   $GPGSA,A,3,02,05,12,25,29,,,,,,,,3.68,1.93,3.13,1*10
   $GPGSV,3,1,12,2,22,057,35,5,62,106,43,9,,,28,10,,,26,1*54
   $GPGSV,3,2,12,12,35,165,41,13,,,25,18,,,36,21,,,26,1*5A
   $GPGSV,3,3,12,25,61,206,43,26,,,26,29,60,332,41,32,,,25,1*68
   $GPRMC,193530.37,A,3719.27869,N,12159.98811,W,0.53,0.00,080721,,,A,V*3C

   Press Button 1 to attempt another fix.
   
Sample output (with A-GPS)
==========================

When booting up, you should see the following printout (extra empty lines have been removed to improve readability).

.. code-block:: console

   *** Booting Zephyr OS build v2.4.99-ncs1  ***
   Starting GPS application
   GPS Socket created
   Getting GPS data...

   Tracking: 0 Using: 0 Unhealthy: 0
   ---------------------------------
   Seconds since last fix: 0
   Searching [|]

   NMEA strings:

   ---------------------------------

   New AGPS data requested, contacting SUPL server, flags 59
   Established LTE link
   ip 8efb:2c0:200:d801:1300:1300:: (c002fb8e) port 7276
   Starting SUPL session
   ULP encoding length: 39
   Bytes sent: 39
   Bytes received: 34, total 34
   ULP ossDecode success, choice 3
   SUPL server responded using version 2.0.4
   SUPL response received
   ULP encoding length: 58
   Bytes sent: 58
   Bytes received: 708, total 708
   ULP ossDecode more input 4
   Bytes received: 2346, total 3054
   ULP ossDecode success, choice 5
   Injected AGPS data, flags: 1, size: 16
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 2, size: 72
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   Injected AGPS data, flags: 3, size: 32
   No integrity data available
   Injected AGPS data, flags: 4, size: 8
   Injected AGPS data, flags: 6, size: 144
   latitude: 3478175
   longitude: -5685508
   altitude: 0
   unc_semimajor: 49
   unc_semiminor: 49
   orientation_major: 0
   unc_altitude: 127
   confidence: 68
   Initially injected location: 37.31676936, -121.99776649
   Injected AGPS data, flags: 7, size: 16
   SUPL POS received
   read again
   Bytes received: 34, total 34
   ULP ossDecode success, choice 6
   SUPLEND:
           Mask: 0
           Status: 0
   SUPL END received
   SUPL session internal resources released
   SUPL session finished
   Done

   Tracking: 0 Using: 0 Unhealthy: 0
   ---------------------------------
   Seconds since last fix: 0
   Searching [/]

   NMEA strings:

   ---------------------------------

Once the fix is realized (typically 1 - 2 seconds in good conditions):

.. code-block:: console

   Tracking: 6 Using: 6 Unhealthy: 0
   ---------------------------------
   Longitude:  -121.999876
   Latitude:   37.321304
   Altitude:   32.800102
   Speed:      0.146501
   Heading:    0.000000
   Date:       2021-07-08
   Time (UTC): 19:40:00
   Initially injected location: 37.31676936, -121.99776649
   Fix location: 37.32130364, -121.99987577

   Time to get fix: 1 seconds
   ---------------------------------
   NMEA strings for GPS fix:

   $GPGGA,194000.97,3719.27822,N,12159.99255,W,1,06,1.41,32.80,M,,M,,*54
   $GPGLL,3719.27822,N,12159.99255,W,194000.97,A,A*74
   $GPGSA,A,3,05,12,18,20,25,29,,,,,,,3.56,1.41,3.26,1*1D
   $GPGSV,4,1,13,2,20,058,,5,62,101,46,11,14,056,,12,33,165,46,1*68
   $GPGSV,4,2,13,13,00,118,,15,05,149,,18,33,255,42,20,46,059,43,1*61
   $GPGSV,4,3,13,23,05,201,,25,59,204,46,26,09,321,,29,61,335,44,1*6D
   $GPGSV,4,4,13,31,11,285,,1*5B
   $GPRMC,194000.97,A,3719.27822,N,12159.99255,W,0.28,0.00,080721,,,A,V*3F

   Press Button 1 to attempt another fix.

Notes
*****

* An external antenna (instead of the on-board antenna) can be used with this example by adding **CONFIG_GPS_SAMPLE_ANTENNA_EXTERNAL=y** to the prj.conf file. This will disable the LNA going to the on-board antenna.
* For best results, use modem firmware v1.3.0 as that introduces GPS improvements

Future Work
***********

* Migration to NCS v1.6.0 with new GPS API
