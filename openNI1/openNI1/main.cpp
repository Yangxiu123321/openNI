#include "stdio.h"
#include <OpenNI.h>

#include "OniSampleUtilities.h"

#define SAMPLE_READ_WAIT_TIMEOUT 2000  //2000ms

using namespace openni;

int main(int argc, char* argv[])
{
	//��ʼ��openNI����
	Status rc = OpenNI::initialize();
	if (rc != STATUS_OK)
	{
		printf("Initialize failed\n%s\n",OpenNI::getExtendedError());
		return 1;
	}

	//��������Device�豸
	Device device;

	if (argc < 2)
		rc = device.open(ANY_DEVICE);
	else
		rc = device.open(argv[1]);

	if (rc != STATUS_OK)
	{
		printf("Couldn't open device\n%s\n",OpenNI::getExtendedError());
		return  2;
	}

	//��������������
	VideoStream depth;

	if (device.getSensorInfo(SENSOR_DEPTH) != NULL)
	{
		rc = depth.create(device,SENSOR_DEPTH);
		if (rc != STATUS_OK)
		{
			printf("Couldn't creat depth stream\n%s\n", OpenNI::getExtendedError());
			return 3;
		}

		rc = depth.start();
		if (rc != STATUS_OK)
		{
			printf("Couldn't start depth stream\n%d\n",OpenNI::getExtendedError());
			return 4;
		}

		//��ȡ��������Ϣ��������VideoFrameRef��
		VideoFrameRef frame;

		while (!wasKeyboardHit())
		{
			int changedStreamDummy;
			VideoStream* pStream = &depth;

			rc = OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, SAMPLE_READ_WAIT_TIMEOUT);
			if (rc != STATUS_OK)
			{
				printf("Wait failed! (timeout is %d ms)\n%s\n", SAMPLE_READ_WAIT_TIMEOUT, OpenNI::getExtendedError());
				continue;
			}

			rc = depth.readFrame(&frame);
			if (rc != STATUS_OK)
			{
				printf("Read failed!\n%s\n", OpenNI::getExtendedError());
				continue;
			}

			if (frame.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_1_MM && frame.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_100_UM)
			{
				printf("Unexpected frame format\n");
				continue;
			}

			DepthPixel* pDepth = (DepthPixel*)frame.getData();

			int middleIndex = (frame.getHeight() + 1)*frame.getWidth() / 2;

			printf("[%08llu] %8d\n", (long long)frame.getTimestamp(), pDepth[middleIndex]);
		}

		depth.stop();
		depth.destroy();
		device.close();
		OpenNI::shutdown();

		return 0;
		}
	}
