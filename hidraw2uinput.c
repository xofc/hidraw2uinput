#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/uinput.h>

/* See 
**	https://github.com/derekhe/wavesahre-7inch-touchscreen-driver
**	http://thiemonge.org/getting-started-with-uinput
*/
char *uinput_dev_str = "/dev/uinput";
char *hidraw_dev_str = "/dev/hidraw0";	/* could be 0 or... */

/*
** calibration (perfect = 0 0 4096 4096)
*/
#define MIN_X	160
#define MIN_Y	300
#define MAX_X	3950
#define MAX_Y	3950

#define NOT_MUCH 80	/* small dx or dy */
#define SHORT_CLICK 250	/* 250 ms */

void crash(char *str)
	{
	perror(str);
	exit(-1);
	}
int now()
	{
	struct timeval tv;
	static int t0;

	if (gettimeofday(&tv, NULL) < 0)
		crash("gettimeofday()");
	if (t0 == 0)
		t0 = tv.tv_sec; /* first call */
	return((tv.tv_sec - t0)*1000 + tv.tv_usec/1000); /* Not foolproof... fails after 11 days up */
	}

struct uinput_user_dev uidev;

int init_uinput()
	{
	int	fd;

	if ((fd = open(uinput_dev_str, O_WRONLY | O_NONBLOCK)) < 0)
		crash(uinput_dev_str);

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0
		|| ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0
		|| ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT) < 0
		|| ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0
		|| ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0
		|| ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0)
		crash("ioctl(UI_SET_*)");

	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "hidraw2uinput");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1; uidev.id.product = 0x1; /* should be something else */
	uidev.id.version = 1;
	uidev.absmin[ABS_X] = MIN_X; uidev.absmax[ABS_X] = MAX_X;
	uidev.absmin[ABS_Y] = MIN_Y; uidev.absmax[ABS_Y] = MAX_Y;

	if (write(fd, &uidev, sizeof(uidev)) < 0)
		crash("write(&uidev)");

	if (ioctl(fd, UI_DEV_CREATE) < 0)
		crash("UI_DEV_CREATE");
	return(fd);
	}
int init_hidraw()
	{
	int	fd;

	if ((fd = open(hidraw_dev_str, O_RDONLY)) < 0)
		crash(hidraw_dev_str);
	return(fd);
	}
int get_raw_event(int fd, int *touchp, int *xp, int *yp)
	{
	unsigned char buf[32];

	if (read(fd, buf, sizeof(buf)) < 22)
		return(0);
	/* typicaly 170   1   1 115  14  73 187   0   0   0   0   0   0... */
	if (buf[0] != 170 || buf[6] != 187)
		{
		fprintf(stderr, "Bad framing from hidraw!\n");
		return(0);	/* something is wrong... */
		}

	*touchp = buf[1];
	*xp = (buf[2] << 8) + buf[3];
	*yp = (buf[4] << 8) + buf[5];
	return(1);
	}
#define abs(a) ((a)>0?(a):-(a))
int main(int argc, char *argv[])
	{
	int	uinput_fd;
	int	hidraw_fd;
	struct input_event     ev[2];
	int	b, x, y;
	int	touch_state, landing_x0, landing_y0, landing_t0, has_moved;

	hidraw_fd = init_hidraw();
	uinput_fd = init_uinput();

	touch_state = 0; /* penUp */
	while (get_raw_event(hidraw_fd, &b, &x, &y))
		{
printf("%d %4d %4d\n", b, x, y);  /* debug */
		if (b == 1)
			{
			memset(&ev, 0, sizeof(struct input_event));
			ev[0].type = ev[1].type = EV_ABS;
			ev[0].code = ABS_X; ev[1].code = ABS_Y;
			ev[0].value = x; ev[1].value = y;
			if(write(uinput_fd,  &ev, sizeof(ev)) < 0)
				crash("event write");
			if (touch_state == 0)
				{ /* landing */
				touch_state = 1; has_moved = 0;
				landing_x0 = x; landing_y0 = y;
				landing_t0 = now();
				}
			else	{
				if (abs((x - landing_x0)) > NOT_MUCH || abs((y - landing_y0)) > NOT_MUCH)
					has_moved = 1;
				}
			}
		else	{
			if (touch_state == 1 && has_moved == 0)
				{ /* 'click' take-off */
				if ((now() - landing_t0) < SHORT_CLICK)
					{ /* short click == left mouse click */
					ev[0].type = EV_KEY;
					ev[0].code = BTN_LEFT;
					ev[0].value = 1;
					if(write(uinput_fd,  &ev[0], sizeof(ev[0])) < 0)
						crash("event write: Left-click");
					ev[0].value = 0;
					if(write(uinput_fd,  &ev[0], sizeof(ev[0])) < 0)
						crash("event write: Left-click");
					printf("left-click\n");
					}
				else	{ /* long click == right mouse click */
					ev[0].type = EV_KEY;
					ev[0].code = BTN_RIGHT;
					ev[0].value = 1;
					if(write(uinput_fd,  &ev[0], sizeof(ev[0])) < 0)
						crash("event write");
					ev[0].value = 0;
					if(write(uinput_fd,  &ev[0], sizeof(ev[0])) < 0)
						crash("event write: Right-click");
					printf("right-click: Right-click\n");
					}
				}
			touch_state = 0;
			}
	        }

	if(ioctl(uinput_fd,  UI_DEV_DESTROY) < 0)
		crash("error: ioctl");
	close(uinput_fd);
	return 0;
	}
