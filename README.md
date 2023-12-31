# IsoBuildHAT

Instructions and a slight modification to run the BuildHAT directly from any TTL serial port

## [BuildHAT README](./README_BUILDHAT.md)

## Limitations

If the BuildHat becomes wedged, you have to manually power cycle it, as the OOB GPIO signaling for reset doesn't work.

## Requirements

You'll need a [GPIO](https://www.adafruit.com/product/2822) [header](https://shop.pimoroni.com/products/pico-header-pack) and a [USB](https://www.adafruit.com/product/954) to [TTL](https://shop.pimoroni.com/products/usb-to-uart-serial-console-cable) adapter

## Instructions

Flip the BuildHAT over and install the header into the GPIO plug.

![Header alignment](/README/1.jpeg?raw=true)

![Install header](/README/2.jpeg?raw=true)

Connect the USB to TTL adapter.  For this adapter, black is GROUND, Green is Tx, White is Rx.

![USB to TTL connections](/README/3.jpeg?raw=true)

Connect the power

![Connect power](/README/4.jpeg?raw=true)

Now run your IsoBuildHAT script on whatever computer the USB to TTL adapter is plugged in to.

You'll need to specify the serial device when you create the Hat object like so.

`hat = Hat(device="/dev/cu.usbserial-1111")`

## TODO

A STL file for mounting BuildHAT...