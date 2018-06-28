#include <stdio.h>
#include <OpenNI.h> 

#include "OniSampleUtilities.h"

#define SAMPLE_READ_WAIT_TIMEOUT 2000 //2000ms

using namespace openni;  // The entire C++ API is available under the openni namespace.


int main(int argc, char* argv[])
{
	// Be sure to call openni::OpenNI::initialize(), to make sure all drivers are loaded 
	// If no drivers are found, this function will fail. If it does, you can get some basic
	// log by calling openni::OpenNI::getExtendedError() (which returns a string) 
	Status rc = OpenNI::initialize();
	if (rc != STATUS_OK)
	{
		// getExtendedError() method returns additional, human-readable information about the error.
		printf("Initialize failed\n%s\n", OpenNI::getExtendedError());
		return 1;
	}

	// Provides an interface to a single sensor device connected to the system. Requires OpenNI 
	// to be initialized before it can be created. Devices provide access to Streams.
	Device device;

	// Device::open():  connects to a physical hardware device
	// This function returns a status code indicating either success or what error occurred.
	if (argc < 2)
		rc = device.open(ANY_DEVICE);
	else
		rc = device.open(argv[1]);

	if (rc != STATUS_OK)
	{
		printf("Couldn't open device\n%s\n", OpenNI::getExtendedError());
		return 2;
	}

	VideoStream depth;  // VideoStream: Abstracts a single video stream. Obtained from a specific Device.

						// Get the SensorInfo for a specific sensor type on this device
						// The SensorInfo is useful for determining which video modes are supported by the sensor
						// Parameters: sensorType of sensor to get information
						// Returns: SensorInfo object corresponding to the sensor type specified,or NULL if such a sensor is not available from this device.
	if (device.getSensorInfo(SENSOR_DEPTH) != NULL)
	{
		// Before VideoStream object can be used, this object must be initialized with the VideoStream::create() function.
		// The create() function requires a valid initialized device. Once created, 
		// you should call the VideoStream::start() function to start the flow of data
		rc = depth.create(device, SENSOR_DEPTH);
		if (rc != STATUS_OK)
		{
			printf("Couldn't create depth stream\n%s\n", OpenNI::getExtendedError());
			return 3;
		}
	}

	rc = depth.start();  // Start data generation from the video stream
	if (rc != STATUS_OK)
	{
		printf("Couldn't start the depth stream\n%s\n", OpenNI::getExtendedError());
		return 4;
	}

	VideoFrameRef frame;  // VideoFrameRef: Abstracts a single video from and related meta-data. Obtained from a specific Stream.

						  // Polling based data reading
	while (!wasKeyboardHit())
	{
		int changedStreamDummy;
		VideoStream* pStream = &depth;

		// A system of polling for stream access can be implemented by using the OpenNI::waitForAnyStream() function. 
		// When called, it blocks until any of the streams in the list have new data available,or the timeout has passed.
		// It then returns a status code and indicates which stream has data available.
		rc = OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, SAMPLE_READ_WAIT_TIMEOUT);
		if (rc != STATUS_OK)
		{
			printf("Wait failed! (timeout is %d ms)\n%s\n", SAMPLE_READ_WAIT_TIMEOUT, OpenNI::getExtendedError());
			continue;
		}

		// Once a VideoStream has been created, data can be read from it directly with the VideoStream::readFrame() function.
		rc = depth.readFrame(&frame);
		if (rc != STATUS_OK)
		{
			printf("Read failed!\n%s\n", OpenNI::getExtendedError());
			continue;
		}

		// getVideoMode() can be used to determine the video mode settings of the sensor
		// This information includes the pixel format and resolution of the image, as well as the frame rate 
		// PIXEL_FORMAT_DEPTH_1_MM: The values are in depth pixel with 1mm accuracy
		if (frame.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_1_MM && frame.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_100_UM)
		{
			printf("Unexpected frame format\n");
			continue;
		}

		// VideoFrameRef::getData() function that returns a pointer directly to the underlying frame data.
		// This will be a void pointer, so it must be cast using the data type of the individual pixels in order to be properly indexed.
		DepthPixel* pDepth = (DepthPixel*)frame.getData();

		// getHeight() and getWidth() functions are provided to easily determine the resolution of the frame
		int middleIndex = (frame.getHeight() + 1) * frame.getWidth() / 2;

		// getTimestamp: Provides timestamp of frame, measured in microseconds from an arbitrary zero
		printf("[%08llu] %8d\n", (long long)frame.getTimestamp(), pDepth[middleIndex]);

		// %md：m为指定的输出字段的宽度。如果数据的位数小于m，则左端补以空格，若大于m，则按实际位数输出
		// 0：有0表示指定空位填0,如省略表示指定空位不填。
		// u格式：以无符号十进制形式输出整数。对长整型可以用"%lu"格式输出
	}

	depth.stop();    // Stop the flow of data

	depth.destroy(); // Destroy the stream

					 // The close() function properly shuts down the hardware device. As a best practice, any device
					 // that is opened should be closed. This will leave the driver and hardware device in a known
					 // state so that future applications will not have difficulty connecting to them.
	device.close();

	// When an application is ready to exit, the OpenNI::shutdown() function 
	// should be called to shutdown all drivers and properly clean up.
	OpenNI::shutdown();

	return 0;
}