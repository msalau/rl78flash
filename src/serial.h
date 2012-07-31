#ifndef SERIAL_H__
#define SERIAL_H__

int serial_open(const char *port);
int serial_set_baud(int fd, int baud);
int serial_set_dtr(int fd, int level);
int serial_write(int fd, const void *buf, int len);
int serial_read(int fd, void *buf, int len);
int serial_sync(int fd);
int serial_flush(int fd);
int serial_close(int fd);

#endif	// SERIAL_H__
