nRF9160: AT Client with Setup Commands (Modified Example)
#########################################################

.. contents::
   :local:
   :depth: 2

Overview and Motivation
***********************

The AT Client example is a simple example showcasing the ability to send AT Commands to the modem.
This modified example has been developed to automatically send commands when booting up to facilitate debugging of connectivity issues.

Changes from Default Example
****************************

* Enables modem trace
* Subscribes to notifications for CEREG, CNEC, CSCON, and CESQ

The default "at_client" example was used as the base for this example so you can see the changes from the example by going through the commits.

Building and running
********************

This example should be built similarly as mentioned in the "at_client" example.

Usage
=====

The usage is the same as the default "at_client" example.
The modem is **not** automatically turned on when booting up so you should still do AT+CFUN=1 when ready.
Additional AT commands can be run at startup by adding them to the at_commands[] array in main.c.

Sample output
=============

When booting up, you should see the following printout.
Once this has been observed, you are ready to use the sample just like the "at_client" sample.

.. code-block:: console

   *** Booting Zephyr OS build v2.4.99-ncs1  ***
   The AT host sample started
   Modem trace and notifications for CEREG, CNEC, CSCON, and CESQ are enabled

After enabling the modem (AT+CFUN=1), you should see additional printouts for the subscribed notifications as can be seen below:

.. code-block:: console

   *** Booting Zephyr OS build v2.4.99-ncs1  ***
   The AT host sample started
   Modem trace and notifications for CEREG, CNEC, CSCON, and CESQ are enabled
   AT+CFUN=1
   OK
   %CESQ: 35,1,17,2
   +CEREG: 2,"1F07","0079AA02",7
   +CSCON: 1,7,4
   +CEREG: 1,"1F07","0079AA02",7,,,"11100000","00100110"
   %CESQ: 30,1,15,2
   %CESQ: 29,1,20,2
   +CSCON: 0,7,4
   %CESQ: 35,1,23,3
   +CSCON: 1,7,4
   %CESQ: 36,1,17,2
   %CESQ: 37,1,22,3
   +CNEC_ESM: 36,1
   +CSCON: 0,7,4
   %CESQ: 36,1,17,2
   
