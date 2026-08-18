#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BAT_STATUS "/sys/class/power_supply/BAT0/status"
#define BAT_CAP    "/sys/class/power_supply/BAT0/capacity"


static const char *fmt = "%a, %d %b — %H:%M";
static int interval = 1;
static int wifi_every = 5;

static void
get_time(char *buf, size_t n)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	strftime(buf, n, fmt, tm);
}

static void
read_file_int(const char *path, int *out)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return;
	if (fscanf(f, "%d", out) != 1)
		*out = -1;
	fclose(f);
}

static void
read_file_str(const char *path, char *buf, size_t n)
{
	FILE *f = fopen(path, "r");
	if (!f) {
		buf[0] = '\0';
		return;
	}
	if (fgets(buf, n, f)) {
		size_t l = strlen(buf);
		if (l > 0 && buf[l - 1] == '\n')
			buf[l - 1] = '\0';
	} else {
		buf[0] = '\0';
	}
	fclose(f);
}

static void
get_battery(char *buf, size_t n)
{
	int cap = -1;
	char status[32] = {0};

	read_file_int(BAT_CAP, &cap);
	read_file_str(BAT_STATUS, status, sizeof(status));

	if (cap < 0) {
		buf[0] = '\0';
		return;
	}

	const char *icon = "BAT";
	if (strcmp(status, "Charging") == 0)
		icon = "CHG";
	else if (strcmp(status, "Full") == 0)
		icon = "FULL";

	snprintf(buf, n, "%s %d%%", icon, cap);
}

/* nmcli -t -f active,ssid,signal dev wifi
 * yes:HomeNetwork:78
 * no:OtherNetwork:40 */
static void
get_wifi(char *buf, size_t n)
{
	FILE *p = popen("nmcli -t -f active,ssid,signal dev wifi 2>/dev/null", "r");
	if (!p) {
		snprintf(buf, n, "WIFI N/A");
		return;
	}

	char line[256];
	int found = 0;

	while (fgets(line, sizeof(line), p)) {
		if (strncmp(line, "yes:", 4) == 0) {
			size_t l = strlen(line);
			if (l > 0 && line[l - 1] == '\n')
				line[--l] = '\0';

			char *ssid = line + 4;
			char *last_colon = strrchr(ssid, ':');
			int signal = -1;

			if (last_colon) {
				signal = atoi(last_colon + 1);
				*last_colon = '\0';
			}

			/* keep ssid short enough that "%s %d%%" always fits n */
			char ssid_trunc[100];
			size_t slen = strlen(ssid);
			if (slen >= sizeof(ssid_trunc))
				slen = sizeof(ssid_trunc) - 1;
			memcpy(ssid_trunc, ssid, slen);
			ssid_trunc[slen] = '\0';

			if (signal >= 0)
				snprintf(buf, n, "%s %d%%", ssid_trunc, signal);
			else
				snprintf(buf, n, "%s", ssid_trunc);

			found = 1;
			break;
		}
	}
	pclose(p);

	if (!found)
		snprintf(buf, n, "offline");
}

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "24hf:i:w:")) != -1) {
		switch (ch) {
		case '2':
			fmt = "%d %b — %H:%M";
			break;
		case 'f':
			fmt = optarg;
			break;
		case 'i':
			interval = atoi(optarg);
			if (interval < 1)
				interval = 1;
			break;
		case 'w':
			wifi_every = atoi(optarg);
			if (wifi_every < 1)
				wifi_every = 1;
			break;
		case 'h':
		default:
			printf("sipbar\n"
			       "usage: %s [-f fmt] [-i seconds] [-w ticks]\n"
			       "\t-f Custom strftime format string for the clock\n"
			       "\t-i Redraw interval in seconds (default 1)\n"
			       "\t-w Poll wifi every N ticks (default 5)\n",
			       argv[0]);
			return ch == 'h' ? EXIT_SUCCESS : EXIT_FAILURE;
		}
	}

	char time_buf[64];
	char bat_buf[32];
	char wifi_buf[128] = "…";
	int tick = 0;

	while (1) {
		get_time(time_buf, sizeof(time_buf));
		get_battery(bat_buf, sizeof(bat_buf));

		if (tick % wifi_every == 0)
			get_wifi(wifi_buf, sizeof(wifi_buf));

		if (bat_buf[0])
			printf("%%{l}%s | %s | %s\n",
			       wifi_buf, time_buf, bat_buf);
		else
			printf("%%{l}%s | %s\n", wifi_buf, time_buf);

		fflush(stdout);
		sleep(interval);
		tick++;
	}

	return EXIT_SUCCESS;
}
