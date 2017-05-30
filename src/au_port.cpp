#include "au_stream_socket.h"
#include <unistd.h>
#include <fcntl.h>

static bool raw_fix_au_port(au_port port) {
	bool ret = true;
	int n, fd;
	fd = open("/tmp/au_port", O_RDWR | O_CREAT);
	if (fd < 0)
		throw std::runtime_error("cant open au_port");

	if (fcntl(fd, F_SETLEASE, F_RDLCK | F_WRLCK))
		throw std::runtime_error("cant lock au_port");

	au_port tmp;
	while (read(fd, &tmp, sizeof(tmp)) == sizeof(tmp)) {
		if (tmp == port) {
			ret = false;
			break;
		}
	}

	if (ret == true)
		if (write(fd, &port, sizeof(port)) != sizeof(port))
			throw std::runtime_error("can't store port");

	if (fcntl(fd, F_SETLEASE, F_UNLCK))
		throw std::runtime_error("cant unlock au_port");

	if (close(fd))
		throw std::runtime_error("can't close au_port");

	return ret;
}

void fix_au_port(au_port port) {
	if (!raw_fix_au_port(port)) {
		throw std::runtime_error("port is already fixed");
	}
}

void free_au_port(au_port port) {
	int n, fd;
	fd = open("/tmp/au_port", O_RDWR | O_CREAT);
	if (fd < 0)
		throw std::runtime_error("cant open au_port");

	if (fcntl(fd, F_SETLEASE, F_RDLCK | F_WRLCK))
		throw std::runtime_error("cant lock au_port");

	au_port tmp;
	while (read(fd, &tmp, sizeof(tmp)) == sizeof(tmp)) {
		if (tmp == port) {
			port = 0;
			if (lseek(fd, -sizeof(port), SEEK_CUR) < 0)
				throw std::runtime_error("can't seek");

			if (write(fd, &port, sizeof(port)) != sizeof(port))
				throw std::runtime_error("can't clear port");
			break;
		}
	}

	if (fcntl(fd, F_SETLEASE, F_UNLCK))
		throw std::runtime_error("cant unlock au_port");

	if (close(fd))
		throw std::runtime_error("can't close au_port");
}
