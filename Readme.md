 **Lora Mainhead T-Beam**



The idea of the project is that the most LORA / ESP projects are based on a one-way communication of data to a central system as the TTN-Mapper. In this project I would like to have a way back to my node.



This project is based on a TTGO-T-BEAM node with a OLED display, although, it can also work without the display. In de logging you get the answer back. The Idea is to get the Mainhead HAM locator, used to know the locator of a HAM-radio amateur, in the display of the node, uding LORA-TTN



**Dataflow**:

The node will wait for a GPS fix. Then send this data using the default format to a TTN application. This application will forward the position via HTTP to a own web-server. This server will answer with the Mainhead locator based on the position and will send an answer back to TTN based on the ' answer-url'  in the http request. this answer will be send back to the node in one of the RX windows after a transmit of the node (first window is on freq of the TX, second on the beacon freq). When locator received, this will be displayd in the oled-display.

After a transmit (incl rx) the node will go in sleep for a while (you can modify this in the code).

The script I am using will also send the position as a APRS message to APRS using Hamnet. If you wat to use this you have to be connected to hamnet and you need to have an access key for APRS.

**Configuration**

In the config file you have to ID coresponding to you device. Because the up and downlink are working in this code you can ude OTAA. This is safer then ABP (and nice to use it...).

If you are using LORA in the arduino IDE for the first time you also have to configure the ' project'  settings in the LMIC config. Define the freq. used. US915, Eu868 etc.

Create a TTN-Application with the decoder as placed in payloadformat.ttn;

Create an integration in the application with a (default) access key, in the URL the url of your webserver, pointing to your perl script, using method post.

Use the application EUIS of this app in youre node. 

In you're own webserver us the script MH_lora.pl. This script will sendback the data to TTN and also prepare a APRS message and send this via HAMNET to APRS (configure youre call, HAMNET ip of the node you will use and you're aprs-access key)





any questions to erikjan @ pa1er . nl

'73 Erik-Jan PA1ER'