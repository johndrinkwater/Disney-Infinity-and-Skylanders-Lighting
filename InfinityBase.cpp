#include "InfinityBase.h"

InfinityBase::InfinityBase() {

}

InfinityBase::~InfinityBase() {

}

InfinityBase::InfinityBase(int deviceId) {

	// printf("Device id: %d\n",deviceId);
	deviceHandler = connect(deviceId);

	messageId = 0;
	messageReply = new uint8_t[256]();
	deviceSerial = new char[14]();

	int retVal = 0;

	if (libusb_kernel_driver_active(deviceHandler, 0) == 1) {
		retVal = libusb_detach_kernel_driver(deviceHandler, 0);
		if (retVal < 0) {
			libusb_close(deviceHandler);
		}
	}

	retVal = libusb_claim_interface(deviceHandler, 0);

	if (retVal != 0) {
		printf("Error code: %d\n",retVal);
		printf("Error name: %s\n",libusb_error_name(retVal));
		exit(1);
	}

	activate();
}

libusb_device_handle* InfinityBase::connect(int deviceId) {

	libusb_device** devices;
	libusb_context* context;
	struct libusb_device_handle* tryDeviceHandler;

	libusb_init(&context);
	int devicesCount = libusb_get_device_list(context, &devices);

	int error;

	struct libusb_device_descriptor descriptor;

	int retVal = libusb_open(devices[deviceId], &tryDeviceHandler);

	libusb_get_device_descriptor(devices[deviceId], &descriptor);

	if (descriptor.idVendor == 0x0e6f && descriptor.idProduct == 0x0129) {

		return tryDeviceHandler;
	}
}

void InfinityBase::activate() {

	uint8_t packet[] = {0,0,0,0,0x28,0x63,0x29,0x20,0x44,0x69,0x73,0x6e,0x65,0x79,0x20,0x32,0x30,0x31,0x33};
	packet[1] = 0x11; // length
	packet[2] = 0x80; // command

	sendPacket(packet);
}

void InfinityBase::listDiscs() {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x02; // length
	packet[2] = 0xa1; // command

	sendPacket(packet);
}

void InfinityBase::getDiscId(uint8_t disc) {

	// As discs are placed on the base, we receive a message to say which platform, and what № they are
	// this disc number is assigned by the base, picking the first unused number upwards from 0
	// with 2 figures, 2 power discs, a skydome and playset, it should count 0,1,2,3,4,5
	// As discs are removed, holes appear in the numbering and it will reuse them
	// Have been able to stably get up to 8 (meaning 9 discs) at once
	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x03; // length
	packet[2] = 0xb4; // command

	packet[4] = disc;

	sendPacket(packet);
}

uint8_t InfinityBase::nextMessage() {

    messageId = (messageId + 1) ^ 256;
	return messageId;
}

void InfinityBase::sendPacket(uint8_t* packet) {

	// All commands are prefixed with
	packet[0] = 0xFF;

	// Set unique incrementing id
	packet[3] = nextMessage();

	// Store command to direct reply to
	messageReply[ packet[3] ] = packet[2];

	// packet[2] = 0x92; // command
	// packet[1] = 0x08; // length

	// packet [4-] is now custom command data, do not touch.
	int checksum = 0;
	for(int l = 0 ; l < packet[1] + 2 ; l++) {
		checksum += packet[l];
	}
	// Append checksum to tail of cmd data
	packet[ packet[1] + 2 ] = checksum & 0xFF;

	sendPreparedPacket( packet );
}

void InfinityBase::sendPreparedPacket(uint8_t* packet) {

	int len;
	int retVal = -1;

	receivePackets();

	while(retVal < 0) {
		retVal = libusb_bulk_transfer(deviceHandler,0x01,packet,32,&len,100);
		receivePackets();
	}

	// if (retVal != 0) {
	// 	printf("Error code: %d\n",retVal);
	// 	printf("Error name: %s\n",libusb_error_name(retVal));
	// 	exit(1);
	// }

}

void InfinityBase::processReceivedPacket(uint8_t* packet) {

	bool printUnknown = false;
	uint8_t messageLength = packet[1];

/*	printf("DBG  ");
	if (packet[0x00] == 0xaa) {
		printf("%02X ", messageReply[ packet[ 0x02 ] ] );
	}
	for(int i = 0 ; i < messageLength+3 ; i++) {
		printf("%02X ",packet[i]);
	}
	printf("\n");*/

	// Packet:
	// [0]	Reply type:  0xAB = disc moved, 0xAA = reply to query
	// [1]  packet size
	// [2]  msgid     (0xAA and 0xAB queries differ here, as 0xAB lacks this detail)
	// [3]  data
	// [size] checksum


	if (packet[0x00] == 0xab) {

		uint8_t platformSetting = packet[2];
		// TODO discover what packet 3 means, typically set to 0x09
		// if it is like the reply to disc listing, it just means padding?
		uint8_t discRef = packet[4];
		uint8_t placedRemoved = packet[5];

		if (placedRemoved == 0x00) {
			printf("TAG ADD ");
		} else {
			printf("TAG REM ");
		}
		printf("PLAT %02X DISC %02X UNK %02X\n", platformSetting, discRef, packet[3] );

		if (placedRemoved == 0x00) {
			getDiscId( discRef );
		}

	} else if (packet[0x00] == 0xaa) {

		// match packet msgid to our queue to discover what we asked for
		uint8_t msgId = packet[ 0x02 ];
		uint8_t msgType = messageReply[ msgId ];

		if ( msgType == 0x80 ) {

			printf("BOOT \n");
			snprintf(deviceSerial, 13, "%02X%02X%02X%02X%02X%02X",
				packet[19], packet[20], packet[21], packet[22], packet[15], packet[16]);
		} else if ( msgType == 0xa1 ) {

			printf("DISC LS ");
			for(int i = 0 ; i < 32 ; i++) {
				printf("%02X ",packet[i]);
			}
			printf("\n{ ");
			for(int i = 3 ; i < packet[1]+2 ; i++) {
				// XXX (byte & 0xF0) >> 4,	byte & 0x0F
				if (packet[i] && packet[i]  != 0x09 ) {
					printf( "%d,%d ", (packet[i] & 0xF0) >> 4, packet[i] & 0x0F );
				}
			}
			printf("} \n");
		} else if ( msgType == 0xb4 ) {

			if ( packet[ 0x03 ] != 0x80 ) {
				printf("GOT TAG ");
				for(int i = 4 ; i < 11 ; i++) {
					printf("%02X",packet[i]);
					if (i != 10)
						printf(":");
				}
				printf("\n");
			} else {
				// You can request disc information from a location that has no disc, so…
				printf("NO  TAG\n");
			}
		} else if ( msgType == 0x90 ) {
			// printf("COL SET \n");
		} else if ( msgType == 0x91 ) {
			printf("COL REQ ");
			for(int i = 3 ; i < messageLength+2 ; i++) {
				printf( "%02X ", packet[i] );
			}
			printf("\n");
		} else if ( msgType == 0x92 ) {
			//printf("COL FADE \n");
		} else if ( msgType == 0x93 ) {
			//printf("COL FLASH \n");
		} else if ( msgType == 0x94 ) {
			//printf("COL RAND \n");
		} else {
			printUnknown = true;
		}
		messageReply[ msgId ] = 0;

	} else {
		printUnknown = true;
	}

	if (printUnknown) {
		printf("UNKNOWN ");
		for(int i = 0 ; i < messageLength+3; i++) {
			printf("%02X ",packet[i]);
		}
		printf("\n");
	}
}

uint16_t InfinityBase::receivePackets() {

	uint16_t packetsReceived;
	int retVal = 0;
	int len = 0;
	uint8_t* packet = new uint8_t[32]();

	while(retVal == 0) {

		retVal = libusb_bulk_transfer(deviceHandler,0x81,packet,32,&len,10);
		if (retVal == 0) {
			processReceivedPacket(packet);
			packetsReceived += 1;
		}
	}

	return packetsReceived;
}


void InfinityBase::setColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x06; // length
	packet[2] = 0x90; // command

	packet[4] = platform;
	packet[5] = r;
	packet[6] = g;
	packet[7] = b;

	sendPacket(packet);
}

void InfinityBase::setColours(bool setPlayset, uint8_t playsetR, uint8_t playsetG, uint8_t playsetB,
								bool setPlayer1, uint8_t player1R, uint8_t player1G, uint8_t player1B,
								bool setPlayer2, uint8_t player2R, uint8_t player2G, uint8_t player2B) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x0e; // length
	packet[2] = 0x95; // command

	packet[4] = setPlayset;
	packet[5] = playsetR;
	packet[6] = playsetG;
	packet[7] = playsetB;
	packet[8] = setPlayer1;
	packet[9] = player1R;
	packet[10]= player1G;
	packet[11]= player1B;
	packet[12]= setPlayer2;
	packet[13]= player2R;
	packet[14]= player2G;
	packet[15]= player2B;

	sendPacket(packet);
}

void InfinityBase::whatColour(uint8_t platform) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x03; // length
	packet[2] = 0x91; // command
	// XXX This is off by one, likely not wanting to answer for all platforms (it should + could with packet size)
	// XXX 0 returns 1, 1 returns 2, 2 returns 3. There appears to be no way to query the status of ALL platforms
	// XXX 4+ return an empty packet
	// XXX this hack is so we can be consistent with using 1 = playset platform, 2 etc.
	if (platform >= 1)
		platform -= 1;
	packet[4] = platform;

	sendPacket(packet);
}

void InfinityBase::fadeColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b) {

	sineWave(platform, 0x10, 0x02, r, g, b);
}

void InfinityBase::sineWave(uint8_t platform, uint8_t animationDuration, uint8_t iterations, uint8_t r, uint8_t g, uint8_t b) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x08; // length
	packet[2] = 0x92; // command

	packet[4] = platform;
	// XXX To best describe this, it is an absolute sine wave, with animationDuration being Pi (width of half a period in time)
	// XXX https://upload.wikimedia.org/wikipedia/commons/4/42/Simple_full-wave_rectified_sine.svg
	// XXX iterations is a bit more confusing, with 1: being the rise to Pi/2, 2: the fall to 0 = Pi, 3: Pi + Pi/2, 4: 2Pi, or two full pulses
	packet[5] = animationDuration;	// 16 (0x10) = 1s, 8 (0x08) = .5, up to 256 (0xFF) = 16s
	packet[6] = iterations;			// 1 = half (fade to dst colour), 2 = full (fade to dst and back to srs), 3 = full + half
	packet[7] = r;
	packet[8] = g;
	packet[9] = b;

	sendPacket(packet);
}

void InfinityBase::sineWaves(bool setPlayset, uint8_t playsetDuration, uint8_t playsetI, uint8_t playsetR, uint8_t playsetG, uint8_t playsetB,
				bool setPlayer1, uint8_t player1Duration, uint8_t player1I, uint8_t player1R, uint8_t player1G, uint8_t player1B,
				bool setPlayer2, uint8_t player2Duration, uint8_t player2I, uint8_t player2R, uint8_t player2G, uint8_t player2B) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x14; // length
	packet[2] = 0x96; // command

	packet[4] = setPlayset;
	packet[5] = playsetDuration;
	packet[6] = playsetI;
	packet[7] = playsetR;
	packet[8] = playsetG;
	packet[9] = playsetB;

	packet[10] = setPlayer1;
	packet[11] = player1Duration;
	packet[12] = player1I;
	packet[13] = player1R;
	packet[14] = player1G;
	packet[15] = player1B;

	packet[16] = setPlayer2;
	packet[17] = player2Duration;
	packet[18] = player2I;
	packet[19] = player2R;
	packet[20] = player2G;
	packet[21] = player2B;

	sendPacket(packet);
}

void InfinityBase::flashColour(uint8_t platform, uint8_t r, uint8_t g, uint8_t b) {

	pulseWave(platform, 0x02, 0x02, 0x06, r, g, b);
}

void InfinityBase::pulseWave(uint8_t platform, uint8_t crestDuration, uint8_t troughDuration, uint8_t iterations,
								uint8_t r, uint8_t g, uint8_t b) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x09; // length
	packet[2] = 0x93; // command

	packet[4] = platform;
	packet[5] = crestDuration;		// duration for on time
	packet[6] = troughDuration;		// duration for off time
	packet[7] = iterations;			// iterations, 1 = half (flash on), 2 = full (flash on/off), 3 = full + half
	packet[8] = r;
	packet[9] = g;
	packet[10] = b;

	sendPacket(packet);
}

void InfinityBase::pulseWaves(bool setPlayset, uint8_t playsetCrest, uint8_t playsetTrough, uint8_t playsetI,
									uint8_t playsetR, uint8_t playsetG, uint8_t playsetB,
								bool setPlayer1, uint8_t player1Crest, uint8_t player1Trough, uint8_t player1I,
									uint8_t player1R, uint8_t player1G, uint8_t player1B,
								bool setPlayer2, uint8_t player2Crest, uint8_t player2Trough, uint8_t player2I,
									uint8_t player2R, uint8_t player2G, uint8_t player2B) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x17; // length
	packet[2] = 0x97; // command

	packet[4] = setPlayset;
	packet[5] = playsetCrest;
	packet[6] = playsetTrough;
	packet[7] = playsetI;
	packet[8] = playsetR;
	packet[9] = playsetG;
	packet[10] = playsetB;

	packet[11] = setPlayer1;
	packet[12] = player1Crest;
	packet[13] = player1Trough;
	packet[14] = player1I;
	packet[15] = player1R;
	packet[16] = player1G;
	packet[17] = player1B;

	packet[18] = setPlayer2;
	packet[19] = player2Crest;
	packet[20] = player2Trough;
	packet[21] = player2I;
	packet[22] = player2R;
	packet[23] = player2G;
	packet[24] = player2B;

	sendPacket(packet);
}

void InfinityBase::fadeRandomColours(uint8_t platform, uint8_t pulseDuration, uint8_t iterations) {

	uint8_t* packet = new uint8_t[32]();

	packet[1] = 0x05; // length
	packet[2] = 0x94; // command

	packet[4] = platform;
	packet[5] = pulseDuration;	// for one full pulse, 16 (0x10) = 1s, 8 (0x08) = .5, up to 256 (0xFF) = 16s
	packet[6] = iterations;		// 1 = half (fade to dst colour), 2 = full (fade to dst and back to srs), 3 = full + half

	sendPacket(packet);
}

