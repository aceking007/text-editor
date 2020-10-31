/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

// struct for editor config
struct editorConfig {
	int screenrows;
	int screencols;
	struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/

// function to handle exiting when error occurs
void die(const char *s) {
	// clear screen and reposition cursor
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	// display error and exit program
	perror(s);
	exit(1);
}

// function to restore canonical mode after exiting program
void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) { 
		die("tcsetattr");
	}
}

// function to enable raw mode
// in raw mode, data is read byte-by-byte and other things
void enableRawMode() {
	// get the current attributes
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);

	// struct for manipulating terminal attributes
	struct termios raw = E.orig_termios;
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

// function to read key presses
char editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die ("read");
	}
	return c;
}

int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** append buffer ***/

// append buffer contains a pointer to our buffer in memory and a length
struct abuf {
	char *b;
	int len;
};

// constructor for append buffer
#define ABUF_INIT {NULL, 0}

// append method for buffer
void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);

	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

// destructor for deallocating dynamic memory
void abFree(struct abuf *ab) {
	free(ab->b);
}

/*** input ***/

// function to process key presses
void editorProcessKeyPress() {
	char c = editorReadKey();

	switch (c) {
		case CTRL_KEY('q'):
			// clear screen and reposition cursor
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);

			// exit the program
			exit(0);
			break;
	}
}

/*** output ***/

// function to draw tildes in empty rows
void editorDrawRows(struct abuf *ab) {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		abAppend(ab, "~", 1);

		// clears the line
		abAppend(ab, "\x1b[K", 3);
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}

// function to refresh editor screen
void editorRefreshScreen() {
	// initialize append buffer
	struct abuf ab = ABUF_INIT;

	// hide the cursor
	abAppend(&ab, "\x1b[?25l", 6);
	// reposition cursor to the start
	abAppend(&ab, "\x1b[1;1H", 6);

	// draw tildes and reposition cursor
	editorDrawRows(&ab);
	abAppend(&ab, "\x1b[H", 3);
	abAppend(&ab, "\x1b[?25h", 6);

	// print out the append buffer to screen and free memory
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

/*** init ***/

void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
	enableRawMode();
	initEditor();

	char c;
	while (1){
		editorRefreshScreen();
		editorProcessKeyPress();
	}

	return 0;
}
