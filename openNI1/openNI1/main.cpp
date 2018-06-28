#include <stdio.h>
#include "OpenNI.h"

#include "OniSampleUtilities.h"

using namespace openni;

// VideoFrameRef class encapsulates all data related to a single frame read from a VideoStream
void analyzeFrame(const VideoFrameRef& frame)
{
	DepthPixel* pDepth;
	RGB888Pixel* pColor;  // This structure stores a single color pixel value(in 24-bit RGB format).

	int middleIndex = (frame.getHeight() + 1)*frame.getWidth() / 2;

	// getVideoMode() can be used to determine the video mode settings of the sensor
	// This information includes the pixel format and resolution of the image, as well as the frame rate 
	switch (frame.getVideoMode().getPixelFormat())
	{
	case PIXEL_FORMAT_DEPTH_1_MM:
	case PIXEL_FORMAT_DEPTH_100_UM:
		// VideoFrameRef::getData() returns a pointer directly to the underlying frame data.
		pDepth = (DepthPixel*)frame.getData();
		printf("[%08llu] %8d\n", (long long)frame.getTimestamp(),
			pDepth[middleIndex]);
		break;
	case PIXEL_FORMAT_RGB888:
		pColor = (RGB888Pixel*)frame.getData();
		printf("[%08llu] 0x%02x%02x%02x\n", (long long)frame.getTimestamp(),
			pColor[middleIndex].r & 0xff,
			pColor[middleIndex].g & 0xff,
			pColor[middleIndex].b & 0xff);
		break;
	default:
		printf("Unknown format\n");
	}
}


// The VideoStream::NewFrameListener class is provided to allow the implementation of event driven frame reading.
// To use it, create a class that inherits from it and implement override the onNewFrame() method
class PrintCallback : public VideoStream::NewFrameListener
{
public:
	void onNewFrame(VideoStream& stream)
	{

		// Once a VideoStream has been created, data can be read from it directly with the VideoStream::readFrame() function.
		stream.readFrame(&m_frame);

		// VideoFrameRef objects are obtained from calling VideoStream::readFrame()
		analyzeFrame(m_frame);
	}
private:
	VideoFrameRef m_frame;
};


class OpenNIDeviceListener : public OpenNI::DeviceConnectedListener,
	public OpenNI::DeviceDisconnectedListener,
	public OpenNI::DeviceStateChangedListener
{
public:
	// All three events provide a pointer to a openNI::DeviceInfo object. This object can be used to get
	// details and identification of the device referred to by the event
	virtual void onDeviceStateChanged(const DeviceInfo* pInfo, DeviceState state)
	{
		printf("Device \"%s\" error state changed to %d\n", pInfo->getUri(), state);
	}

	virtual void onDeviceConnected(const DeviceInfo* pInfo)
	{
		printf("Device \"%s\" connected\n", pInfo->getUri());
	}

	virtual void onDeviceDisconnected(const DeviceInfo* pInfo)
	{
		printf("Device \"%s\" disconnected\n", pInfo->getUri());
	}
};

int main()
{
	Status rc = OpenNI::initialize();
	if (rc != STATUS_OK)
	{
		printf("Initialize failed\n%s\n", OpenNI::getExtendedError());
		return 1;
	}

	OpenNIDeviceListener devicePrinter;

	// OpenNI defines three events: onDeviceConnected, onDeviceDisconnected, and onDeviceStateChanged
	// Listener classes can be added or removed from the list of event handlers
	OpenNI::addDeviceConnectedListener(&devicePrinter);
	OpenNI::addDeviceDisconnectedListener(&devicePrinter);
	OpenNI::addDeviceStateChangedListener(&devicePrinter);

	openni::Array<openni::DeviceInfo> deviceList;
	// OpenNI::enumerateDevices() returns a list of all available devices connected to the system.
	openni::OpenNI::enumerateDevices(&deviceList);
	for (int i = 0; i < deviceList.getSize(); ++i)
	{
		// DeviceInfo::getUri returns te divice URI. URI can be used by Device::open to open a specific device. 
		printf("Device \"%s\" already connected\n", deviceList[i].getUri());
	}

	Device device;
	rc = device.open(ANY_DEVICE);
	if (rc != STATUS_OK)
	{
		printf("Couldn't open device\n%s\n", OpenNI::getExtendedError());
		return 2;
	}

	VideoStream depth;

	if (device.getSensorInfo(SENSOR_DEPTH) != NULL)
	{
		rc = depth.create(device, SENSOR_DEPTH);
		if (rc != STATUS_OK)
		{
			printf("Couldn't create depth stream\n%s\n", OpenNI::getExtendedError());
		}
	}
	rc = depth.start();
	if (rc != STATUS_OK)
	{
		printf("Couldn't start the depth stream\n%s\n", OpenNI::getExtendedError());
	}


	PrintCallback depthPrinter;

	// Register your created class with an active VideoStream using the VideoStream::addNewFrameListener() function.
	// Once this is done, the event handler function you implemented will be called whenever a new frame becomes available.
	depth.addNewFrameListener(&depthPrinter);

	// Wait while we're getting frames through the printer
	while (!wasKeyboardHit())
	{
		Sleep(100);
	}

	// Removes a Listener from this video stream list. The listener removed will no longer receive new frame events from this stream
	depth.removeNewFrameListener(&depthPrinter);


	depth.stop();
	depth.destroy();
	device.close();
	OpenNI::shutdown();

	return 0;
}