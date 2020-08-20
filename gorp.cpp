/* gorp
Michael Zahniser, 19 Dec 2018

GCC Output Reading Program: a terminal program (using ncurses) that parses the
output of a build command and displays a list of warning and error messages,
allowing you to select any message to jump to the line causing the error.
*/

#include "Display.h"

#include <iostream>
#include <string>

using namespace std;

void PrintVersion()
{
	cout << "gorp (GCC Output Reading Program) 0.9.0" << endl;
	cout << endl;
}



void PrintHelp()
{
	cout << "This program parses the output of a build command and displays any warnings or" << endl;
	cout << "errors in the terminal. Click on a message to jump to the file that produced it." << endl;
	cout << "You can also select messages with the up/down keys and the enter key." << endl;
	cout << "Command line arguments:" << endl;
	cout << "  -v/--version: Display the version number of the program, then exit." << endl;
	cout << "  -h/--help: Display this help message, then exit." << endl;
	cout << "  -c/--commands: Display the command strings, then exit." << endl;
	cout << endl;
	cout << "To customize the build and clean commands, create a \".gorp\" file either in the" << endl;
	cout << "current directory (for project-specific commands) or your home directory (to" << endl;
	cout << "define the default commands to use). The file should include three lines:" << endl;
	cout << "  build: <command>" << endl;
	cout << "  clean: <command>" << endl;
	cout << "  edit: <command>" << endl;
	cout << "The edit command should use \"FILE\", \"LINE\", and \"COLUMN\" as placeholders for the" << endl;
	cout << "file path, line index, and column index (if supported)." << endl;
	cout << endl;
	cout << "If no commands are given via a \".gorp\" file, the defaults are:" << endl;
	cout << "  build: make" << endl;
	cout << "  clean: make clean" << endl;
	cout << "  edit: gedit FILE +LINE:COLUMN" << endl;
	cout << endl;
}



int main(int argc, char *argv[])
{
	// Parse the command lines.
	bool displayCommands = false;
	for(char **it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if(arg == "-v" || arg == "-V" || arg == "--version")
		{
			PrintVersion();
			return 0;
		}
		else if(arg == "-c" || arg == "--commands")
			displayCommands = true;
		else
		{
			PrintHelp();
			return 0;
		}
	}
	
	// Initialize the display, and launch the command (unless all we're doing is
	// parsing settings files to determine what commands to use).
	Display display;
	display.Init(displayCommands);
	if(displayCommands)
		return 0;
	
	// Run the program.
	while(display.Update())
		continue;
	
	// Clean up the terminal.
	display.Cleanup();
	
	return 0;
}
