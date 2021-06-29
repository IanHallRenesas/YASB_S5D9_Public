# YASB_S5D9_Public

**Introduction**

The YASB (Yet Another Synergy Bootloader) bootloader is a secure bootloader for Synergy devices which supports cryptographically signed update images. Signing an image can ensure it originated from a trusted source and has not been altered since being signed by that trusted source. A Python program is used to sign update images. The signature uses ECSDA (Elliptic Curve Digital Signature Algorithm) with the ECC curve secp256k1 and SHA256 hashing.

What this bootloader can do:
* Only allow official signed images to update the device and run on the device.
* Prevent images which have not been signed or signed with an invalid key to update and run on the device.
* Provide integrity that an image has not changed since being signed.

What this bootloader cannot do:

* Protect the image contents as images are not encrypted. So, the image contents could be loaded on a different device with a different boot program and run.

It is expected that updates will be delivered over a trusted medium and once delivered to the device are considered secure.
There is no download functionality built into the bootloader. It is expected that the application will download a new signed image into the update area and reset the device. On starting, the bootloader will look in the update area for a new valid image and update as necessary.

Two projects are supplied

* Bootloader - The bootloader application itself
* PK_S5D9_BL_Blinky - An example blinky project compatible with the bootloader

Both projects use SSP v2.0.0 and e2studio 2021-04

Python is required and the image tool has been tested with Python v3.8.3

A description of the bootloader, it's operation including build instructions for both projects can be found in the Documentation folder.
