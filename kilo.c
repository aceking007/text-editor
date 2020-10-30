#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// struct to store original terminal attributes
struct termios orig_termios;

// function to restore canonical mode after exiting program
void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// function to enable raw mode
// in raw mode, data is read byte-by-byte and other things
void enableRawMode() {
	// get the current attributes
	tcgetattr(STDIN_FILENO, &orig_termios);
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

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
	enableRawMode();

	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {
			printf("%d ('%c')\r\n", c, c);
		}
	}
	return 0;
}
