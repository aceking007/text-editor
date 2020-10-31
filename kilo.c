/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

// struct to store original terminal attributes
struct termios orig_termios;

/*** terminal ***/

void die(const char *s) {
	perror(s);
	exit(1);
}

// function to restore canonical mode after exiting program
void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) { 
		die("tcsetattr");
	}
}

// function to enable raw mode
// in raw mode, data is read byte-by-byte and other things
void enableRawMode() {
	// get the current attributes
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);

	// struct for manipulating terminal attributes
	struct termios raw = orig_termios;
	// the following flags turn off the corresponding items:
 
	// ixon -- ctrl-s and ctrl-q = XOFF and XON
	// icrnl -- ctrl-m = carriage return
	// brkint -- sends sigint signals to the program
	// inpck -- enables parity checking
	// istrip -- strips 8th bit of each input byte
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);

	// opost -- turns off output processing
	raw.c_oflag &= ~(OPOST);
	
	// cs8 -- sets character size to 8 bits per byte
	raw.c_cflag |= (CS8);
	
	// echo -- showing output as the user types 
	// icanon -- canonical mode = reading input line-by-line
	// iexten -- ctrl-v
	// isig -- sigint signals
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	// vmin sets the min no of bytes of input needed before read can return
	raw.c_cc[VMIN] = 0;
	// vtime sets max time to wait before read returns (unit 1/10 seconds)
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

/*** init ***/

int main() {
	enableRawMode();

	char c;
	while (1){
		char c = '\0';
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
			die("read");
		}
		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {
			printf("%d ('%c')\r\n", c, c);
		}
		if (c == CTRL_KEY('q')) break;
	}

	return 0;
}
