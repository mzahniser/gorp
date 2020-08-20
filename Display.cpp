/* Display.cpp
Michael Zahniser, 19 Dec 2018
*/

#include "Display.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include <locale.h>
#include <ncursesw/curses.h>

using namespace std;



// Initialize the display (and begin the build).
void Display::Init(bool displayCommands)
{
	// Check for a .gorp file specifying the commands to use.
	LoadCommands(getenv("HOME") + string("/.gorp"));
	LoadCommands(".gorp");
	
	if(displayCommands)
	{
		cout << "build: " << buildCommand << endl;
		cout << "clean: " << cleanCommand << endl;
		cout << "edit: " << editCommand << endl;
		return;
	}
	
	// Determine what the build and clean commands should be.
	Launch(buildCommand);
	
	// ncurses terminal setup.
	setlocale(LC_CTYPE,"");
	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, true);
	keypad(stdscr, true);
	mousemask(ALL_MOUSE_EVENTS, nullptr);
	mouseinterval(0);
	curs_set(0);
	
	// Set up the colors we'll be using.
	use_default_colors();
	start_color();
	init_pair(COLOR_RED, COLOR_RED, -1);
	init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
	init_pair(COLOR_CYAN, COLOR_CYAN, -1);
}



// Redraw the screen, then wait for the next event or input from the build
// process and handle it appropriately. Return false if it's time to quit.
bool Display::Update()
{
	// If a quit event is received, tell the program to quit.
	if(!HandleEvents())
		return false;
	
	ParseOutput();
	
	return true;
}



// Clean up, returning the terminal to "cooked" mode.
void Display::Cleanup()
{
	curs_set(1);
	endwin();
}



// Check for a .gorp file specifying the commands to use.
void Display::LoadCommands(const string &path)
{
	ifstream in(path);
	if(!in)
		return;
	
	string line;
	while(getline(in, line))
	{
		// Parse the line into a tag and a command.
		size_t pos = line.find(':') + 1;
		string tag = line.substr(0, pos);
		// Skip whitespace.
		while(pos < line.length() && line[pos] <= ' ')
			++pos;
		string command = line.substr(pos);
		
		if(tag == "build:")
			buildCommand = command;
		else if(tag == "clean:")
			cleanCommand = command;
		else if(tag == "edit:")
			editCommand = command;
	}
}



// Launch the given command, after cleaning up previous data.
void Display::Launch(const string &command)
{
	// Clean up.
	messages.clear();
	messageCount.clear();
	output.clear();
	errorLocation.clear();
	previousError.clear();
	errorLinesAfter = 0;
	selectedIndex = -1;
	scrollIndex = 0;
	
	// Launch the new command.
	title = command;
	process.Start(command);
	
	// Until the command is done running, ncurses should run in polling mode
	// instead of blocking mode.
	nodelay(stdscr, true);
}



void Display::Draw()
{
	// Get the size of the terminal.
	getmaxyx(stdscr, lines, columns);
	
	// Display the current output line.
	attron(A_REVERSE);
	DrawText(title, 0);
	attroff(A_REVERSE);
	
	if(messages.empty())
		DrawOutput();
	else
		DrawErrors();
	
	// Clear to the end of the screen, then refresh the screen.
	clrtobot();
	refresh();
}



void Display::DrawOutput()
{
	// Scroll so that the most recent line of output is always visible.
	vector<string>::const_iterator it = output.begin() + max<int>(0, output.size() - (lines - 1));
	
	for(int line = 1; line < lines && it != output.end(); ++it, ++line)
		DrawText(*it, line);
}



void Display::DrawErrors()
{
	// Display any messages received so far.
	int line = 0;
	for(int index = scrollIndex; index < static_cast<int>(messages.size()) && line < lines; ++index)
	{
		const Message &message = messages[index];
		bool isSelected = (index == selectedIndex);
		int color = COLOR_PAIR(message.Color());
		attron(color);
		bool first = true;
		for(const string &text : message.Text())
		{
			// Stop drawing if we've reached the bottom of the screen.
			if(++line >= lines)
				break;
			DrawText(text, line);
			
			if(first)
			{
				first = false;
				// Only the first line should be drawn in color.
				attroff(color);
				// Subsequent lines should be highlighted if this line is selected.
				if(isSelected)
					attron(A_REVERSE);
			}
		}
		if(first)
			attroff(color);
		if(isSelected)
			attroff(A_REVERSE);
	}
}



void Display::DrawText(string text, int line)
{
	// First, find the length of the string, which may contain UTF-8.
	int length = 0;
	size_t pos = 0;
	for( ; pos < text.length() && length < columns; ++pos)
		length += ((text[pos] & 0b11000000) != 0b10000000);
	
	// If the string is longer than the number of columns, truncate it. If it is
	// shorter, pad it with spaces.
	if(length < columns)
		text.append(columns - length, ' ');
	else if(pos < text.length())
		text.erase(pos);
	
	// Now, display the string.
	move(line, 0);
	addstr(text.data());
}



bool Display::HandleEvents()
{
	while(true)
	{
		Draw();
		
		// If the command is still running, parse its output.
		int input = getch();
		if(input == ERR)
			return true;
		
		// This is set if a message should be "opened" either because the return
		// key was pressed or a message was clicked.
		int openIndex = -1;
		
		// Switch based on the input.
		if(input == KEY_DOWN)
		{
			selectedIndex = min<int>(messages.size() - 1, selectedIndex + 1);
			while(selectedIndex > LastVisibleIndex())
				ScrollDown();
		}
		else if(input == KEY_UP)
		{
			selectedIndex = max(-1, selectedIndex - 1);
			while(selectedIndex >= 0 && selectedIndex < scrollIndex)
				ScrollUp();
		}
		else if(input == KEY_PPAGE)
		{
			int offset = selectedIndex - scrollIndex;
			ScrollUp();
			selectedIndex = scrollIndex + offset;
		}
		else if(input == KEY_NPAGE)
		{
			int offset = selectedIndex - scrollIndex;
			ScrollDown();
			selectedIndex = scrollIndex + offset;
		}
		else if(input == 'q')
			return false;
		else if(input == '\n')
			openIndex = selectedIndex;
		else if(input == ' ')
		{
			Launch(buildCommand);
			isCleaning = false;
		}
		else if(input == KEY_BACKSPACE || input == KEY_DL)
		{
			Launch(cleanCommand);
			isCleaning = true;
		}
		else if(input == KEY_MOUSE)
		{
			MEVENT event;
			if(getmouse(&event) == OK)
			{
				if(event.bstate & BUTTON1_PRESSED)
				{
					int index = LineIndex(event.y);
					if(index != static_cast<int>(messages.size()))
						openIndex = selectedIndex = index;
				}
				// Scroll wheel handling requires ncurses6.
				else if(event.bstate & BUTTON4_CLICKED)
					scrollIndex = max(0, scrollIndex - 1);
				else if(event.bstate & BUTTON5_CLICKED)
					scrollIndex = min(static_cast<int>(messages.size()) - 1, scrollIndex + 1);
			}
		}
		
		// If we're supposed to open a file, send the command to open it.
		if(openIndex >= 0 && !messages[openIndex].File().empty())
		{
			const Message &message = messages[openIndex];
			
			map<string, string> sub;
			sub["FILE"] = message.File();
			sub["LINE"] = (message.Line() > 0) ? to_string(message.Line()) : "1";
			sub["COLUMN"] = (message.Column() > 0) ? to_string(message.Column()) : "1";
			
			// Always launch the command in an independent shell.
			string command = "(" + editCommand + " &)";
			for(const pair<string, string> &it : sub)
			{
				size_t pos = command.find(it.first);
				if(pos != string::npos)
					command.replace(pos, it.first.length(), it.second);
			}
			system(command.data());
		}
	}
}



// Get the index of the last visible message.
int Display::LastVisibleIndex() const
{
	return LineIndex(lines - 1);
}



// Get the maximum scroll index. This is the lowest index where the last
// message would be fully on-screen.
int Display::MaxScrollIndex() const
{
	return PageBefore(messages.size());
}



// Get the index of the message displayed on the given line. If line 0 is given,
// this returns -1. If a line after the last message is given, this returns the
// size of the messages vector.
int Display::LineIndex(int line) const
{
	// Special cases: line 0 has no message on it, and line 1 is the scroll index.
	if(line <= 1)
		return (line == 1 ? scrollIndex : -1);
	
	int currentLine = 1;
	int index = scrollIndex;
	int size = messages.size();
	while(index < size)
	{
		currentLine += messages[index].Text().size();
		if(currentLine > line)
			break;
		++index;
	}
	return index;
}



// Get the index of the message that's a page before the given index.
int Display::PageBefore(int index) const
{
	int line = lines - 1;
	while(index > 0)
	{
		line -= messages[index - 1].Text().size();
		if(line < 1)
			break;
		--index;
	}
	return index;
}



// Scroll up by a full page.
void Display::ScrollUp()
{
	// Scroll so that the item at the top of the page becomes the last one
	// visible on the bottom.
	scrollIndex = PageBefore(scrollIndex);
}



// Scroll down by a full page.
void Display::ScrollDown()
{
	// Scroll so that the item at the bottom of the page is now at the top.
	scrollIndex = min(MaxScrollIndex(), LastVisibleIndex());
}



// Parse any messages from the process.
void Display::ParseOutput()
{
	// No need to do anything below this if the process is no longer running.
	if(process.IsDone())
		return;
	
	// If there's any error output, process it and then return.
	string text = process.ReadError();
	if(!text.empty())
	{
		output.push_back(text);
		
		Message::Type type = Message::NONE;
		// Ignore linker errors. Find "undefined reference" instead.
		if(text.find(": error: ld returned") != string::npos)
			type = Message::NONE;
		else if(text.find(": undefined reference") != string::npos)
			type = Message::LINK;
		else if(text.find(": error: ") != string::npos)
			type = Message::ERROR;
		else if(text.find(": warning: ") != string::npos)
			type = Message::WARNING;
		++messageCount[type];
	
		if(type == Message::LINK)
			messages.emplace_back(type, "Linker error:", text);
		else if(type != Message::NONE)
		{
			// Parse this line and the one before it to see if the previous
			// line is stating the location of the error.
			size_t pos = text.find(':') + 1;
			if(previousError.length() > pos
					&& previousError[pos] == ' '
					&& !text.compare(0, pos, previousError, 0, pos))
				errorLocation = previousError.substr(pos + 1);
			errorLinesAfter = 2;
		
			messages.emplace_back(type, errorLocation, text);
		}
		else if(errorLinesAfter)
		{
			messages.back().AddText(text);
			--errorLinesAfter;
		}
		previousError = text;
		return;
	}
	// If there was no error output but there is regular output, process it and
	// then return.
	text = process.ReadOutput();
	if(!text.empty())
	{
		output.push_back(text);
		if(!messages.empty())
			title = text;
		return;
	}
	
	// If this last set of reads was the last output, continue on from here.
	if(!process.IsDone())
		return;
	
	// If we got here, the process is done.
	// Once we're done building, getch() can be called in blocking mode
	// instead of polling mode.
	nodelay(stdscr, false);
	
	// If we're running a clean command, no need to summarize errors.
	if(isCleaning)
	{
		title = "Done cleaning.";
		return;
	}
	
	// Set the title to indicate the number of errors.
	int errors = messageCount[Message::ERROR];
	int warnings = messageCount[Message::WARNING];
	int linkErrors = messageCount[Message::LINK];
	
	ostringstream out;
	out << "Done building (";
	if(errors && warnings)
		out << errors << " errors, " << warnings << " warnings";
	else if(errors)
		out << errors << " errors";
	else if(warnings && linkErrors)
		out << warnings << " warnings, " << linkErrors << " link errors";
	else if(warnings)
		out << warnings << " warnings";
	else if(linkErrors)
		out << linkErrors << " link errors";
	else
		out << "no errors";
	out << ").";
	title = out.str();
}
