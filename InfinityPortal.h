#ifndef INFINITYPORTAL_H
#define	INFINITYPORTAL_H

#include <stdio.h>
#include "libusb-1.0/libusb.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <unistd.h>

class InfinityPortal {
public:
	InfinityPortal();
	InfinityPortal(int deviceId);
	virtual ~InfinityPortal();
	libusb_device_handle* deviceHandler;
	void setColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b);
	void flashColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b);
	void fadeColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b);
	void activate();
	void getTagId();
private:
	void processReceivedPacket(uint8_t* packet);
	void sendPacket(uint8_t* packet);
	void sendPreparedPacket(uint8_t* packet);
	uint16_t receivePackets();
	libusb_device_handle* connect(int deviceId);
};

#endif
