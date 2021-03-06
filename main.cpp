/* 
 * File:   main.cpp
 * Author: christopher
 *
 * Created on 10 August 2014, 21:09
 */

#include <cstdlib>
#include <stdio.h>
#include <sys/types.h>
#include <iostream>
#include <math.h>
#include "libusb-1.0/libusb.h"
#include <unistd.h>
#include "InfinityBase.h"
#include "SkylandersPortal.h"

using namespace std;

/*
 * 
 */
int main(int argc, char** argv) {

	srand (time(NULL));

	libusb_device** devices;
	libusb_context* context;
	struct libusb_device_handle* tryDeviceHandler;

	libusb_init(&context);
	int devicesCount = libusb_get_device_list(context, &devices);

	struct libusb_device_descriptor descriptor;
	libusb_device_handle* deviceHandler;

	int retVal = 0;
	int skylanderPortalIds[0xff];
	int infinityBaseIds[0xff];
	int skylanderPortalCount = 0;
	int infinityBaseCount = 0;

	for(int i = 0 ; i < devicesCount ; i++) {

			retVal = libusb_open(devices[i], &tryDeviceHandler);

			if(retVal < 0) {
					continue;
			}

			libusb_get_device_descriptor(devices[i], &descriptor);

			if(descriptor.idVendor == 0x0e6f && descriptor.idProduct == 0x0129) {

					printf("Found infinity portal at %d\n",i);

					infinityBaseIds[infinityBaseCount] = i;
					infinityBaseCount++;

			} else if(descriptor.idVendor == 0x1430 && descriptor.idProduct == 0x150) {

					printf("Found skylander portal at %d\n",i);

					skylanderPortalIds[skylanderPortalCount] = i;
					skylanderPortalCount++;
			}
	}

	if(skylanderPortalCount == 0 && infinityBaseCount == 0) {
		printf("Please plug in either a Portal of Power or an Infinity Base\n");
		return -1;
	}

	InfinityBase infinityBases[infinityBaseCount];
	SkylandersPortal skylanderPortals[skylanderPortalCount];

	int j;

	for(j = 0 ; j < infinityBaseCount ; j++) {
		infinityBases[j] = InfinityBase(infinityBaseIds[j]);
	}

	for(j = 0 ; j < skylanderPortalCount ; j++) {
		skylanderPortals[j] = SkylandersPortal(skylanderPortalIds[j]);
	}

	printf("Starting light show\n");

	while(true) {

		for(j = 0 ; j < max(skylanderPortalCount,infinityBaseCount) ; j++) {

			if(j < skylanderPortalCount) {
				// printf("Doing %d\n",j);
				skylanderPortals[j].setColour(random()%0x100,random()%0x100,random()%0x100);
				// skylanderPortals[j].setLeftColour(random()%0x100,random()%0x100,random()%0x100);
				// skylanderPortals[j].setRightColour(random()%0x100,random()%0x100,random()%0x100);
				// skylanderPortals[j].flashTrapLight();
				// skylanderPortals[j].writeMaskerMind();
				return 0;
			}

			if(j < infinityBaseCount) {
				for(int k = 0 ; k < 3 ; k++) {
					infinityBases[j].setColour(k+1,random()%0x100,random()%0x100,random()%0x100);
				}
			}
		}

		usleep(100000);
	}


	printf("Done!\n");
	return 0;
}
