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
#include <signal.h>
#include <unistd.h>

#include <glib.h>

#include <asm/types.h>
#include <linux/hiddev.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "libmx5000/mx5000.h"
#include "input_translate.h"

GMainLoop *mainloop = NULL;


void signal_handler(int signal)
{
  g_main_loop_quit(mainloop);
}

int main(int argc, char **argv)
{

  int fd;
  GError *err = NULL;
  pid_t pid;

  static gchar *device = NULL;
  //  static gchar *path = "/dev/usb";
  static gboolean foreground = 0;

  GOptionContext *goptcontext;


  static const GOptionEntry entries[] = {
    { "device", 'd', 0, G_OPTION_ARG_FILENAME, &device, "Hiddev device", "dev" },
    { "foreground", 'f', 0, G_OPTION_ARG_NONE, &foreground, "Keep in foreground", NULL},
    //    { "directory", 'p', 0, G_OPTION_ARG_FILENAME, &device, "Directory where to look for hiddev devices", "dir" },
    { NULL }
  };


  goptcontext = g_option_context_new ("MX5000 Daemon");
        
  g_option_context_add_main_entries (goptcontext, entries, NULL);

  if(!g_option_context_parse(goptcontext, &argc, &argv, &err)) {
    g_error("error: %s\n", err->message);
    return 1;
  }
  
  if (device) {
    fd = mx5000_open_path(device);
  } else {
    fd = mx5000_open();
  }
  
  if (fd < 0) {
    g_error("Could not open hiddev device\n");
    return 1;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  

  if (init_input_translate(fd) < 0) {
    g_error("Could not setup input translation\n");
    goto out;
  }

  if (!foreground) {
    pid = fork();
    if (pid > 0) {
      return 0;
    } else if (pid < 0) {
      perror("Error forking");
      return 0;
    }

    close(0);
    close(1);
    close(2);
  }

  mainloop = g_main_loop_new(NULL, FALSE);

  g_main_loop_run(mainloop);

 out:
  destroy_input_translate();

  close(fd);

  return 0;
}

