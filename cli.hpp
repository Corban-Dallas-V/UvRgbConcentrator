// ----------------------------------------------------------------------------

#include <modm/board.hpp>

#include <modm/debug.hpp>

using namespace Board;
// ----------------------------------------------------------------------------

/// @brief simple CLI class
class Cli {
	friend class Tcs;
	friend class V6040;
	friend class V6070;

	enum { CMD_LINE_LENGTH = 80, CMD_MAX_ARGC = 10 };

public:
	enum class Cmd: uint8_t {
		// Controls:
		None,
		Control,
		Command,
		Error
	};

	// Results IDs
	enum class Result: uint8_t {
		eDone,
		eOk,
		eWarning,
		eError,
		eInvalidCommand,
		eInvalidArg,
		eInvalidKey,
		eWrongArgNumbers,
		eInsufficientMemory,
		eHelp
	};

public:

	Cli(modm::IOStream& s):
		_ios(s)					{
		_control		= 0;
		_commands.clear();
		_capacity		= 0;
		_ctlHasTaken	= false;
		_lastInputState	= Cmd::None;
		_argc			= 0;
		_argv[0][0]		= '\0';
	}

	void
	prompt() const				{ _ios << _prompt; }

	std::string
	command() const				{ return std::string(_argv[0]); }

	char
	control() const				{ return _control; }

	bool
	isDone()					{ return _capacity == 0; }

	void
	done()						{
		clearCommands();
		prompt();
	}

	void
	usage() {
		_ios <<
				"Usage:\n"
				"	Name_of_sensor Command [Option<n>]:		command line\n"
				"	Available sensors:\n"
				"		tcs | v6040 | v6070\n"
				"	Available commands:\n"
				"	Common:\n"
				"		Ctrl+C | Esc:						stop the polling\n"
				"		[-r | --restart]:					restart sensor polling\n"
				"	For TCS:\n"
				"		Wlong:								set Wlong bit\n"
				"		Wtime:								set Wtime to W\n"
				"		Atime:								set Atime to A\n"
				"		Again:								set Gain  to G\n"
				"	For VEML6040:\n"
				"		Atime:								set Atime to A\n"
				"	For VEML6070:\n"
				"		Atime:								set Atime to A\n"
				"\n";
		prompt();
	}

	Cmd
	checkInput()		{
		char	buf[2];

		if ( _ctlHasTaken && _capacity )
			return	_lastInputState;

		while(true) {
			_ios.get(buf, 2);

			if ( buf[0] )	_ios << buf[0];

			if ( ( buf[0] == 0 ) ||
				( buf[0] == modm::IOStream::eof )
			) {
				_lastInputState	= Cmd::None;
				return	Cmd::None;
			}

			if (( buf[0] != CR ) && ( buf[0] != LF ))
				_commands += buf[0];

			if ( buf[0] == BackSpace ) {
				_commands.pop_back();
				_commands.pop_back();
			}

			if ( ( buf[0] > 0 ) && ( buf[0] < Space ) )
				break;
		}

		if ( isCtrls(_control) ) {
			_ctlHasTaken	= true;
			_lastInputState	= Cmd::Control;
			return	Cmd::Control;
		} else if (( buf[0] == CR ) || ( buf[0] == LF )) {
			if ( parseCmdLine(_commands.c_str()) == Result::eDone ) {
				_ctlHasTaken	= true;
				_lastInputState	= Cmd::Command;
				return	Cmd::Command;
			} else {
				clearCommands();
				_ios << "\nUnknown command" << modm::endl;
				usage();
			}
		}

		return Cmd::None;
	}

private:

	const char Ctrl_A	= {0x01};
	const char Ctrl_C	= {0x03};
	const char Ctrl_D	= {0x04};
	const char BackSpace= {0x08};
	const char Ctrl_P	= {0x10};
	const char Ctrl_Q	= {0x11};
	const char LF		= {0x0A};
	const char CR		= {0x0D};
	const char Esc		= {0x1B};
	const char Space	= {0x20};

	void
	clearCommands()	{
		_commands.clear();
		_ctlHasTaken	= false;
		_argc			= 0;
		_argv[0][0]		= '\0';
	}

	Result
	parseCmdLine(const char* cmd_line);

	bool
	isCtrls(char& ctl) {
		for (uint8_t i = 0; i < _commands.length(); ++i) {
			if ( ( _commands[i] > 0 ) && ( _commands[i] < Space ) ) {
				ctl		= _commands[i];
				return true;
			}
		}
		return false;
	}

	modm::IOStream& _ios;
	const char*		_prompt		= "\n/>";	// CLI prompt
	char			_control;				// CLI control (Ex. Ctrl+C)
	std::string		_commands;				// CLI command (from terminal)
	uint8_t			_capacity;				// CLI command queue capacity for threads/processes
	bool			_ctlHasTaken;			// CLI command has been taken
	Cmd				_lastInputState;		// CLI input state
	uint8_t		 	_argc;
	char			_argv[CMD_MAX_ARGC][CMD_LINE_LENGTH];
};
// ----------------------------------------------------------------------------

// Cmd line parsing:
Cli::Result
Cli::parseCmdLine(const char* cmd_line) {
	const
	char	*p	= cmd_line;
	char	s[CMD_LINE_LENGTH];						// temp string
	uint8_t	i	= 0;								// columns (in _argv)
	uint8_t l	= 1;								// lines (in _argv)
													// FSM states:
	enum	{ cmd, arg_lead_space, arg, eerror }
	eFsm	= cmd;

	Result											// parsing result
	res		= Result::eDone;

	if ( strlen(cmd_line) >= Cli::CMD_LINE_LENGTH ) {
		res	= Result::eError;
		_ios	<< "Achtung! Command line exceeds maximum (" << Cli::CMD_LINE_LENGTH << " symbols)\n";
		return res;
	}

	while( *p ){
		switch(eFsm){
		case cmd:
			if( !isalnum(*p) && ( *p != '-' ) && ( *p != ' ' )) {
				res			= Result::eInvalidCommand;
				eFsm		= eerror;				// not a good symbol
				break;
			}
			if(( *p == '-' ) && ( i == 0 )){		// leading hyphen
				res		= Result::eInvalidCommand;
				eFsm	= eerror;
				break;
			}
			if(( *p == ' ' ) || ( *p == '\t' )){
				if ( i == 0 ) break;				// leading spaces
				else {								// the end of 1st word (command name)
					s[i++]	= ';' ;					// splitter
					++l;							// prepare for _argc
					eFsm	= arg_lead_space;
					break;
				}
			}
			s[i++]			= *p;					// commands symbols
			break;
		case arg_lead_space:
			if(( *p == ' ' ) || ( *p == '\t' )) 	// leading spaces
				break;
			else {
				s[i++]		= *p;
				eFsm		= arg;
			}
			break;
		case arg:
			if(( *p == ' ' ) || ( *p == '\t' )) {
				s[i++]	= ';' ;						// splitter
				++l;
				eFsm	= arg_lead_space;
				break;
			}
			s[i++]			= *p;					// parameters symbols
			break;
		case eerror:
			// do nothing: (wait of end of incoming string)
			break;
		}
		++p;
	}
	if( s[i] != ';') {
		s[i]= ';' ;
		++l;
	}
	s[++i]	= '\0';

	if( res != Result::eDone) {						// parsing wrong
		return res;
	}

	if( i > 0 ) {									// coming to the end of command string, but not meet any symbols
		if( _argc > 0 ) {							// delete old _argc and _argv
			for (uint8_t cnt = 0; cnt < _argc; cnt++)
				if(_argv[cnt] != nullptr)
					_argv[cnt][0] = '\0';
		}
		_argc		= l-1;							// 1 - name of program (we doesn't have it in mC)
		if ( _argc >= Cli::CMD_MAX_ARGC ) {
			_argc	= Cli::CMD_MAX_ARGC - 1;
			_ios	<< "Achtung! Arguments number exceeds maximum (" << Cli::CMD_MAX_ARGC << ")\n";
		}

		l			= 0;
		p			= s;
		uint8_t
		c			= 0;

		while( l < _argc) {
			i		= 0;
			while( s[c] && ( s[c] != ';' )) {
				++i; ++c;
			}

			strncpy(_argv[l], p, i);
			_argv[l][i]	= '\0';
			p			= &s[++c];
			++l;
		}
	}
	return res;										// parsing is done
}
// ----------------------------------------------------------------------------

#include <getopt.h>
#include <map>

char *strlwr(char *str);							// #include don't see strlwr

class CommandBase {
public:
	explicit
	CommandBase(Cli& cli): _cli(cli){}

	virtual
	~CommandBase()	= default;

	virtual void
	getOptions() {}

protected:
		bool			fverbose	= false;
		bool			fhelp		= false;
		bool			ferror		= false;

		Cli&			_cli;
		std::map<std::string, std::string>
						_messages	= {
							{"help",	"See usage"},
							{"invArg",	"Invalid argument"},
							{"ok",		"Done"}
						};
};
// ----------------------------------------------------------------------------
/*__PRETTY_FUNCTION__ */

class Tcs: public CommandBase {
public:
	Tcs(Cli&	cli): CommandBase(cli) {}

	void
	getOptions() override {

		const struct option loptions[] = {
			{"wtime",		required_argument,	NULL, 'w'},
			{"atime",		required_argument,	NULL, 'a'},
			{"again",		required_argument,	NULL, 'g'},
			{"wlong",		no_argument,		NULL, 'l'},
			{"init",		no_argument,		NULL, 'i'},
			{"ping",		no_argument,		NULL, 'p'},
			{"restart",		no_argument,		NULL, 'r'},
			{"verbose",		no_argument,		NULL, 'v'},
			{"help",		no_argument,		NULL, 'h'},
			{0,0,0,0}
		};

		int opt;
		opterr				= 0;
		optarg				= nullptr;
		optind				= 0;

		char*const* av;
		char*		p[Cli::CMD_MAX_ARGC];

		for(uint8_t i=0; i<_cli._argc; i++) p[i]	= _cli._argv[i];
		av					= p;
																		/* Debug msg {
		_cli._ios << "argc: " << _cli._argc << " argv: ";
		for(uint8_t i=0; i<_cli._argc; i++) _cli._ios << av[i] << " ";
		_cli._ios << "end of argv\n";									}*/

		while ( (opt = getopt_long(_cli._argc, av, "w:a:g:liprvh", loptions, NULL)) != -1 ) {
	        switch (opt) {
	        case 'w':
	        	swtime		= optarg;
	        	break;
	        case 'a':
	        	satime		= optarg;
	        	break;
	        case 'g':
	        	sagain		= optarg;
	        	break;
	        case 'l':
	        	wlong		= true;
	        	break;
	        case 'i':
	        	init		= true;
	        	break;
	        case 'p':
	        	ping		= true;
	        	break;
	        case 'r':
	        	restart		= true;
	        	break;
	        case 'v':
	            fverbose   	= true;
	            break;
	        case 'h':
	            fhelp   	= true;
	            break;
	        case ':':
	            ferror 		= true;
	            break;
	        case '?':
	            ferror		= true;
	            break;
	        }
	    }

	    if( ferror ) {
	    	_cli._ios << (_messages["invArg"]).c_str() << modm::endl;
	    }
	    if( fhelp ){
	    	_cli._ios << (_messages["help"]).c_str() << modm::endl;
	    }

	}

	bool			restart		= false;
	bool			ping		= false;
	bool			init		= false;
	bool			wlong		= false;
	std::string		sagain;						// X1/X4/X16/X60
	std::string		swtime;						// 0..256 (0 = 256(614ms/7.4s); 0xFF = 1(2,4ms/0.029s)wo long bit/w long bit)
	std::string		satime;						// Count = (256 − ATIME) × 1024 up to a maximum of 65535 (0 = 700ms; 0xFF = 2.4ms)
};
// ----------------------------------------------------------------------------

class V6040: public CommandBase {
public:
	V6040(Cli&	cli): CommandBase(cli) {}

	void
	getOptions() override {

		const struct option loptions[] = {
			{"atime",		required_argument,	NULL, 'a'},
			{"init",		no_argument,		NULL, 'i'},
			{"ping",		no_argument,		NULL, 'p'},
			{"restart",		no_argument,		NULL, 'r'},
			{"verbose",		no_argument,		NULL, 'v'},
			{"help",		no_argument,		NULL, 'h'},
			{0,0,0,0}
		};

		int opt;
		opterr				= 0;
		optarg				= nullptr;
		optind				= 0;

		char*const* av;
		char*		p[Cli::CMD_MAX_ARGC];

		for(uint8_t i=0; i<_cli._argc; i++) p[i]	= _cli._argv[i];
		av					= p;

		while ( (opt = getopt_long(_cli._argc, av, "a:iprvh", loptions, NULL)) != -1 ) {
	        switch (opt) {
	        case 'a':
	        	satime		= optarg;
	        	break;
	        case 'i':
	        	init		= true;
	        	break;
	        case 'p':
	        	ping		= true;
	        	break;
	        case 'r':
	        	restart		= true;
	        	break;
	        case 'v':
	            fverbose   	= true;
	            break;
	        case 'h':
	            fhelp   	= true;
	            break;
	        case ':':
	            ferror 		= true;
	            break;
	        case '?':
	            ferror		= true;
	            break;
	        }
	    }

	    if( ferror ) {
	    	_cli._ios << (_messages["invArg"]).c_str() << modm::endl;
	    }
	    if( fhelp ){
	    	_cli._ios << (_messages["help"]).c_str() << modm::endl;
	    }

	}

	bool			restart		= false;
	bool			ping		= false;
	bool			init		= false;
	std::string		satime;						// 1280ms 640ms 320ms 160ms 80ms  40ms
};
// ----------------------------------------------------------------------------

class V6070: public CommandBase {
public:
	V6070(Cli&	cli): CommandBase(cli) {}

	void
	getOptions() override {

		const struct option loptions[] = {
			{"atime",		required_argument,	NULL, 'a'},
			{"init",		no_argument,		NULL, 'i'},
			{"ping",		no_argument,		NULL, 'p'},
			{"restart",		no_argument,		NULL, 'r'},
			{"verbose",		no_argument,		NULL, 'v'},
			{"help",		no_argument,		NULL, 'h'},
			{0,0,0,0}
		};

		int opt;
		opterr				= 0;
		optarg				= nullptr;
		optind				= 0;

		char*const* av;
		char*		p[Cli::CMD_MAX_ARGC];

		for(uint8_t i=0; i<_cli._argc; i++) p[i]	= _cli._argv[i];
		av					= p;

		while ( (opt = getopt_long(_cli._argc, av, "a:iprvh", loptions, NULL)) != -1 ) {
	        switch (opt) {
	        case 'a':
	        	satime		= optarg;
	        	break;
	        case 'i':
	        	init		= true;
	        	break;
	        case 'p':
	        	ping		= true;
	        	break;
	        case 'r':
	        	restart		= true;
	        	break;
	        case 'v':
	            fverbose   	= true;
	            break;
	        case 'h':
	            fhelp   	= true;
	            break;
	        case ':':
	            ferror 		= true;
	            break;
	        case '?':
	            ferror		= true;
	            break;
	        }
	    }

	    if( ferror ) {
	    	_cli._ios << (_messages["invArg"]).c_str() << modm::endl;
	    }
	    if( fhelp ){
	    	_cli._ios << (_messages["help"]).c_str() << modm::endl;
	    }

	}

	bool			restart		= false;
	bool			ping		= false;
	bool			init		= false;
	std::string		satime;						// 500ms 250ms 125ms 62.5ms
};
// ----------------------------------------------------------------------------
