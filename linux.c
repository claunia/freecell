/*
  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stddef.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "linux.h"

char *drive;
int drivefd;

int getdrive(char *arg) {
  int drivenumber;
  char *endptr;
   
  if (strlen(arg) < 7 || strlen(arg) > 10) return 1;
  if (strncmp("/dev/sr", arg, 6) != 0) return 1;
  if (arg[7] < '0' || arg[7] > '9') return 1;
  if (arg[7] == '0' && arg[8] != 0) return 1;
  drivenumber = strtoul(&arg[7], &endptr, 10);
  if (endptr[0] != 0) return 1;
  if (drivenumber > 255) return 1;

  drive = arg;
  return 0;
}

int opendrive() {
  if ((drivefd = open(drive, O_RDONLY | O_NONBLOCK)) < 0) {
    fprintf(stderr, EXECUTABLE "Could not open drive %s.\n", drive);
    return 1;
  }
  return 0;
}

void closedrive() {
  close(drivefd);
}

unsigned long long millisecondstime() {
  struct timeval currenttime;
  time_t unixtime = time(NULL);
  gettimeofday(&currenttime, NULL);
  return ((unixtime * 1000) + (currenttime.tv_usec / 1000));
}

int sendcdb(const unsigned char *cdb, unsigned char cdblength, unsigned char *buffer, size_t size, int in, unsigned int *sense) {
  struct sg_io_hdr io_hdr;
  unsigned char sensebuffer[32];

  memset(&io_hdr, 0, sizeof(io_hdr));
  memset(&sensebuffer, 0, 32);

  io_hdr.interface_id = 'S';
  io_hdr.cmdp = (unsigned char *)cdb;
  io_hdr.cmd_len = cdblength;
  io_hdr.dxferp = buffer;
  io_hdr.dxfer_len = size;
  io_hdr.dxfer_direction = (in ? SG_DXFER_FROM_DEV : SG_DXFER_TO_DEV);
  io_hdr.sbp = sensebuffer;
  io_hdr.mx_sb_len = sizeof(sensebuffer);
  io_hdr.timeout = 20000;

  if (ioctl(drivefd, SG_IO, &io_hdr) < 0) {
    *sense = 1;
    return 1;
  }

  *sense = ((sensebuffer[2] & 0x0f) << 16) + (sensebuffer[12] << 8) + sensebuffer[13];
  return 0;
}
