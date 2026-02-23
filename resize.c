#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/ioctl.h>

int main() {
	struct winsize ws;
	struct termios term;
	char buf[9];
	int params[3] = {0, 0, 0};
	int bufpos = 0, p = 0;
	int fdi = fileno(stdin);
	int fdo = fileno(stdout);
	char c = '\0';
	const char* err = NULL;

	/* canary just in case */
	buf[8] = '\0';

	/* verify the input is a tty */
	if (!isatty(fdo)) {
		fprintf(stderr, "STDOUT is not a TTY!\n");
		return 2;
	}

	/* setup the terminal for ansi code readback */
	tcgetattr(fdi, &term);
	term.c_lflag &= ~ECHO;
	term.c_lflag &= ~ICANON;
	tcsetattr(fdi, 0, &term);

	/* send the escape code to get terminal size */
	if (write(fdo, "\x1b[18t", 5) != 5) {
		err = "Failed to write the ANSI control code to read terminal size\n";
		goto fail;
	}

	/* read / parse the terminal size */
	while (read(fdi, &c, 1) > 0) {
		switch (c) {
			case 't':
			case ';':
				buf[bufpos] = '\0';
				bufpos = 0;
				if (p >= 3) {
					err = "Too many parameters received!\n";
					goto fail;
				}

				params[p++] = atoi(buf);
				break;
			default:
				if (!isdigit(c))
					break;
				buf[bufpos++] = c;
				if (bufpos >= 8) {
					err = "Too many characters received!\n";
					goto fail;
				}
				break;
		}
		if (c == 't')
			break;
	}

fail:
	/* reset the terminal to its normal state */
	term.c_lflag |= ECHO;
	term.c_lflag |= ICANON;
	tcsetattr(fdi, 0, &term);

	/* verify the received values are normal */
	if (params[0] != 8 || params[1] <= 0 || params[2] <= 0)
		err = "Invalid parameter received\n";

	/* set the terminal size */
	if (err == NULL) {
		ws.ws_row = params[1];
		ws.ws_col = params[2];
		if (ioctl(fdo, TIOCSWINSZ, &ws) == -1) {
			fprintf(stderr, "TIOCSWINSZ ioctl failed!\n");
			return 3;
		}
		return 0;
	} else {
		fputs(err, stderr);
		return 1;
	}
}
