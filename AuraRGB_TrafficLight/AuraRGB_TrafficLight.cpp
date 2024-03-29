#include <stdio.h>
#include <stdint.h>

#include <Windows.h>
#import "libid:F1AA5209-5217-4B82-BA7E-A68198999AFA" // AURA SDK 3

#define _TRAFFIC_LIGHT_RED 0xff0000
#define _TRAFFIC_LIGHT_GREEN 0x00ff00
#define _TRAFFIC_LIGHT_YELLOW 0xafaf00
#define _LED_UPDATE_INTERVAL_MS 20

struct _CTX_TRAFFIC_LIGHT_VARS {
	int red_s;
	int yellow_s;
	int green_s;
	int blink_ms;
	int red_countdown_s;
	int green_countdown_s;
};

static int init_traffic_vars(void);
static void start_trafficlight_color_thread(void);
static int main_led_color_thread(void);
static int lightup_traffic_led(void);

static void print_traffic_vars(void);

static uint8_t rgb_led[3] = { 0 };
static _CTX_TRAFFIC_LIGHT_VARS rgb_timing;

int main(int argc, char *argv[])
{
	printf("Init led values...\n");

	init_traffic_vars();
	print_traffic_vars();

	printf("Starting main thread...\n");

	lightup_traffic_led();

	return 0;
}

static void print_traffic_vars(void)
{
	printf("\tGreen light total time: %d sec.\n", rgb_timing.green_s);
	printf("\tYellow light total time: %d sec.\n", rgb_timing.yellow_s);
	printf("\tRed light total time: %d sec.\n", rgb_timing.red_s);
	printf("\tGreen light count down time: %d sec.\n", rgb_timing.green_countdown_s);
	printf("\tRed light count down time: %d sec.\n", rgb_timing.red_countdown_s);
	printf("\tLight blink time: %d ms.\n", rgb_timing.blink_ms);
}

static int init_traffic_vars(void)
{
	memset(&rgb_timing, 0, sizeof(_CTX_TRAFFIC_LIGHT_VARS));

	rgb_timing.red_countdown_s = 13;
	rgb_timing.green_countdown_s = 11;
	rgb_timing.blink_ms = 500;

	rgb_timing.red_s = 30;
	rgb_timing.yellow_s = 3;
	rgb_timing.green_s = 20;

	return 0;
}

static void start_trafficlight_color_thread(void)
{
	HANDLE th = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main_led_color_thread, NULL, 0, NULL);
}

static int main_led_color_thread(void)
{
	int time_red_solid = (rgb_timing.red_s - rgb_timing.red_countdown_s) * 1000,
		time_red_blinked = rgb_timing.red_countdown_s * 1000,
		time_blink = rgb_timing.blink_ms,
		time_green_solid = (rgb_timing.green_s - rgb_timing.green_countdown_s) * 1000,
		time_green_blinked = (rgb_timing.green_countdown_s - 5) * 1000,
		time_yellow_solid = rgb_timing.yellow_s * 1000;

	auto _set_rgb = [](unsigned char *a, uint32_t rgb, int sleep_ms) {
		a[0] = (rgb >> 16) & 0xff; a[1] = (rgb >> 8) & 0xff; a[2] = rgb & 0xff;
		Sleep(sleep_ms);
	};

	while (1) {
		// solid red
		_set_rgb(rgb_led, _TRAFFIC_LIGHT_RED, time_red_solid);

		// red blink
		_set_rgb(rgb_led, 0x000000, time_blink);
		// solid red
		_set_rgb(rgb_led, _TRAFFIC_LIGHT_RED, time_red_blinked - time_blink);

		// solid green
		_set_rgb(rgb_led, _TRAFFIC_LIGHT_GREEN, time_green_solid);

		// green blink
		_set_rgb(rgb_led, 0x000000, time_blink);
		// solid green
		_set_rgb(rgb_led, _TRAFFIC_LIGHT_GREEN, 5000);

		// green last 5 sec
		_set_rgb(rgb_led, 0x000000, time_blink); _set_rgb(rgb_led, _TRAFFIC_LIGHT_GREEN, 1000 - time_blink);
		_set_rgb(rgb_led, 0x000000, time_blink); _set_rgb(rgb_led, _TRAFFIC_LIGHT_GREEN, 1000 - time_blink);
		_set_rgb(rgb_led, 0x000000, time_blink); _set_rgb(rgb_led, _TRAFFIC_LIGHT_GREEN, 1000 - time_blink);
		_set_rgb(rgb_led, 0x000000, time_blink); _set_rgb(rgb_led, _TRAFFIC_LIGHT_GREEN, 1000 - time_blink);
		_set_rgb(rgb_led, 0x000000, time_blink); _set_rgb(rgb_led, _TRAFFIC_LIGHT_GREEN, 1000 - time_blink);

		// solid yellow
		_set_rgb(rgb_led, _TRAFFIC_LIGHT_YELLOW, time_yellow_solid);
	}
	return 0;
}

static int lightup_traffic_led(void)
{
	HRESULT hr;
	// Initialize COM
	hr = ::CoInitialize(nullptr);
	if (SUCCEEDED(hr))
	{
		AuraServiceLib::IAuraSdkPtr sdk = nullptr;
		hr = sdk.CreateInstance(__uuidof(AuraServiceLib::AuraSdk), nullptr, CLSCTX_INPROC_SERVER);
		if (SUCCEEDED(hr))
		{
			// Acquire control
			sdk->SwitchMode();
			// Enumerate all devices
			AuraServiceLib::IAuraSyncDeviceCollectionPtr devices;
			devices = sdk->Enumerate(0); // 0 means ALL
			// devices = sdk->Enumerate(0x00010000L); // Motherboard only

			printf("Device count: %d\n", devices->Count);

			auto _get_rgb_value = [](uint8_t *a, int b) {
				// { R, B, G } to 0x00GGBBRR
				return (a[3 * b] | (a[3 * b + 1] << 8) | (a[3 * b + 2] << 16)) & 0x00FFFFFF;
			};

			start_trafficlight_color_thread();

			while (devices->Count > 0) {

				for (int i = 0; i < devices->Count; i++)
				{
					AuraServiceLib::IAuraSyncDevicePtr dev = devices->Item[i];

					// printf("Device %08X: '%s'\n", dev->GetType(), (LPCTSTR) dev->GetName());

					AuraServiceLib::IAuraRgbLightCollectionPtr lights = dev->Lights;
					for (int j = 0; j < lights->Count; j++)
					{
						AuraServiceLib::IAuraRgbLightPtr light = lights->Item[j];
						light->Color = _get_rgb_value(rgb_led, 0);
					}

					dev->Apply();
				}

				if (_LED_UPDATE_INTERVAL_MS > 0) Sleep(_LED_UPDATE_INTERVAL_MS);

			}

		}
	}

	::CoUninitialize();

	return 0;
}
