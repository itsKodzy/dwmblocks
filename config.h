//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/*Icon*/	/*Command*/		/*Update Interval*/	/*Update Signal*/
	{"",	    "sb-moonphase",	1,	                  9},
	{"",	    "sb-playerctl",	1,	                  8},
	{"",	    "sb-memory",	1,	                  7},
	{"",	    "sb-cpubars",	1,	                  6},
	{"",	    "sb-usagecpu",	1,	                  5},
	{"",	    "sb-nettraf",	1,	                  2},
	{"",	    "sb-packages",	60,	                  4},
	{"",	    "sb-volume",	0,	                  3},
	{"",	    "sb-clock",	    60,	                  1},
	// {"",	"sb-tasks",	10,	26},
	/* {"",	"sb-music",	0,	11}, */
	// {"",	"sb-pacpackages",	0,	8},
	// {"",	"sb-news",		0,	6},
	/* {"",	"sb-price xmr-btc \"Monero to Bitcoin\" 🔒 25",	9000,	25}, */
	/* {"",	"sb-price xmr Monero 🔒 24",			9000,	24}, */
	/* {"",	"sb-price eth Ethereum 🍸 23",			9000,	23}, */
	/* {"",	"sb-price btc Bitcoin 💰 21",			9000,	21}, */
	// {"",	"sb-torrent",	20,	7},
	/* {"",	"sb-memory",	10,	14}, */
	/* {"",	"sb-cpu",		10,	18}, */
	/* {"",	"sb-moonphase",	18000,	17}, */
	// {"",	"sb-doppler",	0,	13},
	// {"",	"sb-forecast",	18000,	5},
	// {"",	"sb-mailbox",	180,	12},
	// {"",	"sb-battery",	5,	3},
	// {"",	"sb-internet",	5,	4},
	/* {"",	"sb-iplocate", 0,	27}, */
	// {"",	"sb-help-icon",	0,	15},
};

//Sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char *delim = " ∘ ";

// Have dwmblocks automatically recompile and run when you edit this file in
// vim with the following line in your vimrc/init.vim:

// autocmd BufWritePost ~/.local/src/dwmblocks/config.h !cd ~/.local/src/dwmblocks/; sudo make install && { killall -q dwmblocks;setsid dwmblocks & }
