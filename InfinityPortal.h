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
	char *deviceSerial;
	uint8_t messageId;
	uint8_t *messageReply;
	void setColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b);
	void whatColour(uint8_t platform);
	void flashColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b);
	void pulseWave(uint8_t platform, uint8_t crestDuration, uint8_t troughDuration, uint8_t iterations, uint8_t r, uint8_t g, uint8_t b);
	void fadeRandomColours(uint8_t platform, uint8_t pulseDuration, uint8_t iterations);
	void fadeColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b);
	void sineWave(uint8_t platform, uint8_t animationDuration, uint8_t iterations, uint8_t r, uint8_t g, uint8_t b);
	void activate();
	void getDiscId(uint8_t disc);
	void listDiscs();
private:
	void processReceivedPacket(uint8_t* packet);
	uint8_t nextMessage();
	void sendPacket(uint8_t* packet);
	void sendPreparedPacket(uint8_t* packet);
	uint16_t receivePackets();
	libusb_device_handle* connect(int deviceId);
};

#endif
