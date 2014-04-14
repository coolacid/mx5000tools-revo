/* mx5000tools
 * Copyright (C) 2006 Olivier Crete
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <glib.h>

#include <asm/types.h>
#include <linux/hiddev.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "libmx5000/mx5000.h"

int uinput_fd = -1;

int mediakeys = 1;

void send_uinput_event(int type, int code, int value)
{
  struct input_event ievent;
  int rv;

  if (uinput_fd < 0) {
    return;
  }

  ievent.type = type;
  ievent.code = code;
  ievent.value = value;

  rv = write(uinput_fd, &ievent, sizeof(ievent));
  if (rv < 0) {
    perror("Injecting event");
  }
  printf("injected\n");
}


gboolean eventin(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
  struct hiddev_event hevent;
  gsize size;
  GError *err = NULL;
  int rv = 0;

  rv = g_io_channel_read_chars(ioc, (gchar*)&hevent, sizeof(hevent), &size, &err);
  if (rv != G_IO_STATUS_NORMAL) {
    if (err) {
      g_warning("Reading  events failed (rv:%d): %s\n", rv, err->message);
    } else {
      g_warning("Reading events failed rv:%d\n", rv);
    }
    return TRUE;
  }

  if (size != sizeof(hevent)) {
    g_warning("The read size isn't what was expected: size:%lu != sizeof(hevent):%lu\n", (unsigned long) size, sizeof(hevent));
    //g_main_loop_quit(mainloop);
    exit(1);
  }

  g_print("Read hid: %x value: %d\n", hevent.hid, hevent.value);

  switch(hevent.hid) {
  case 0xc022d: // Zoom up
    if (hevent.value == 1) {
     send_uinput_event(EV_REL, REL_WHEEL, 1);
     send_uinput_event(EV_SYN, SYN_REPORT, 0);
    }
    break;
 case 0xc022e: // Zoom down
   if (hevent.value == 1) {
     send_uinput_event(EV_REL, REL_WHEEL, -1);
     send_uinput_event(EV_SYN, SYN_REPORT, 0);
   }
    break;
  case 0xc0230: // Zoom 100%
    send_uinput_event(EV_KEY, KEY_ZOOM, hevent.value);
    break;
  case 0xc01b8: // my Videos (digital library library)
    //send_uinput_event(EV_KEY, KEY_VIDEO, hevent.value);
    if (hevent.value == 0)
      break;
    if (mediakeys) {
      printf("disabling media keys\n");
      mediakeys = 0;
      mx5000_set_kbd_opts(g_io_channel_unix_get_fd(ioc), DISABLE_MEDIA_KEYS);
    } else {
      printf("ENABLING media keys\n");
      mediakeys = 1;
      mx5000_set_kbd_opts(g_io_channel_unix_get_fd(ioc), ENABLE_EVERYTHING);
    }
    break;
  case 0xc01b7: // my Music (digital library library)
    send_uinput_event(EV_KEY, KEY_MEDIA, hevent.value);
    break;
  case 0xc01b6: // my Pictures (digital library library)
    send_uinput_event(EV_KEY, KEY_SHOP, hevent.value);
    break;
  case 0xc01bc: // IM
    send_uinput_event(EV_KEY, KEY_CHAT, hevent.value);
    break;

    // F keys in non-F mode
    // Other F keys either are handled by evdev dont send anything
  case 0xc0184: // Word
    send_uinput_event(EV_KEY, KEY_FN_F2, hevent.value);
    break;
  case 0xc0186: // Excel
    send_uinput_event(EV_KEY, KEY_FN_F3, hevent.value);
    break;
  case 0xc0188: // Powerpoint
    send_uinput_event(EV_KEY, KEY_FN_F4, hevent.value);
    break;
    

  default:
    break;
    //g_print("Ignoring hid: %x\n", hevent.hid);
  }


  return TRUE;
}

gboolean pollerr(GIOChannel *ioc, GIOCondition cond, gpointer data)
{

  g_error("Dongle disconnected\n");
  
  //g_main_loop_quit(mainloop);
  exit(1);

  return TRUE;
}

int setup_uinput(int fd)
{
  struct uinput_user_dev uinput_udev;
  struct hiddev_devinfo hdevinfo;
  int rv;



  uinput_fd = open("/dev/uinput", O_RDWR);
  if (uinput_fd < 0) {
    uinput_fd = open("/dev/input/uinput", O_RDWR);
    if (uinput_fd < 0) {
      uinput_fd = open("/dev/misc/uinput", O_RDWR);
      if (uinput_fd < 0) {
	g_error("Can't open input device: %s (%d)",
		strerror(errno), errno);
	return -1;
      }
    }
  }



  rv = ioctl(fd, HIDIOCGDEVINFO, &hdevinfo);
  if (rv < 0) {
    perror("Could not get devinfo from hiddev");
    return -1;
  }
  
  memset(&uinput_udev, 0,  sizeof(uinput_udev));

  strcpy(uinput_udev.name, "mx5000d");

/*   uinput_udev.id.bustype = hdevinfo.bustype; */
/*   uinput_udev.id.vendor = hdevinfo.vendor; */
/*   uinput_udev.id.product = hdevinfo.product; */
/*   uinput_udev.id.version = hdevinfo.version; */


  rv = write(uinput_fd, &uinput_udev, sizeof(uinput_udev));

  if (rv != sizeof(uinput_udev)) {
    g_error("Could not write uinput_user_dev struct\n");
    return -1;
  }

  ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
  ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);

  ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_ZOOM);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_VIDEO);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_MEDIA);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_CAMERA);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_CHAT);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_FN_F2);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_FN_F3);
  ioctl(uinput_fd, UI_SET_KEYBIT, KEY_FN_F4);  
  
  rv = ioctl(uinput_fd, UI_DEV_CREATE, 0);
  if (rv < 0) {
    perror("Could not create uinput device");
    return -1;
  }

  return 0;
}




int init_input_translate(int fd)
{
  GIOChannel *ioc;
  GError *err = NULL;

  ioc = g_io_channel_unix_new(fd);

  g_io_channel_set_encoding(ioc, NULL, &err);


  mx5000_set_kbd_opts(fd, ENABLE_EVERYTHING);

  g_io_add_watch(ioc, G_IO_IN|G_IO_PRI, eventin, NULL);

  g_io_add_watch(ioc, G_IO_ERR|G_IO_HUP, pollerr, NULL);
  

  if (setup_uinput(fd) < 0) {
    g_error("Could not open uinput\n");
    return -1;
  }

  return 0;
}

void destroy_input_translate(void)
{
  if (uinput_fd >= 0) {
    ioctl(uinput_fd, UI_DEV_CREATE, 0);
    close(uinput_fd);
  }
}


