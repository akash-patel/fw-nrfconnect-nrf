.. _send_sms_sample:

nRF9160: Send SMS
##################

The Send SMS example demonstrates how to send an SMS from the nRF9160 modem using the AT+CMGS command.

Overview
********
1) Initiate the modem: `AT+CFUN`_\=1

2) Register modem as a client for mobile-terminated SMS and status reports: `AT+CNMI`_\=3,2,0,1

3) Send the message "Hello from Nordic!" to +16464007658, SMSC = NULL: `AT+CMGS`_\=29\\r0001000B916164622635F5000012C8329BFD0699E5EF36C8F99693D3E310\\x1A

.. _AT+CFUN: https://infocenter.nordicsemi.com/topic/ref_at_commands/REF/at_commands/mob_termination_ctrl_status/cfun_set.html
.. _AT+CNMI: https://infocenter.nordicsemi.com/topic/ref_at_commands/REF/at_commands/text_mode/cnmi_set.html
.. _AT+CMGS: https://infocenter.nordicsemi.com/topic/ref_at_commands/REF/at_commands/text_mode/cmgs_set.html

Requirements
************
- nRF9160 DK
- SIM Card capable of sending SMS, I used a Verizon SIM in my testing **(Included iBasis eSIM with DK does not support SMS natively)**

Running
*******
This example can be run on the nRF9160 DK using the pre-built merged.hex file. Make sure that the programming switch (SW5) is set to nRF91 on the DK and run the following command:

`nrfjprog`_ --program merged.hex --sectorerase --verify -r

.. _nrfjprog: https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools

Notes
*****
- Payload for SMS can be changed by using the tool here: http://rednaxela.net/pdu.php
- +16462662535 can be used for free on https://receive-smss.com/sms/16462662535/ This service may be down from time to time or the number may become invalid so it is advised to use your own personal number.
