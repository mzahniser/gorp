/* Display.h
Michael Zahniser, 19 Dec 2018

Class that reads build script output, displays it, and responds to mouse and
keyboard events to let the user select an error to view.
*/

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "Message.h"
#include "Process.h"

#include <map>
#include <string>
#include <vector>

using namespace std;



class Display {
public:
	// Initialize the display (and begin the build).
	void Init(bool displayCommands);
	// Redraw the screen, then wait for the next event or input from the build
	// process and handle it appropriately. Return false if it's time to quit.
	bool Update();
	// Clean up, returning the terminal to "cooked" mode.
	void Cleanup();
	
	
private:
	// Check for a .gorp file specifying the commands to use.
	void LoadCommands(const string &path);
	// Launch the given command, after cleaning up previous data.
	void Launch(const string &command);
	
	// Display the messages.
	void Draw();
	void DrawOutput();
	void DrawErrors();
	void DrawText(string text, int line);
	
	// Handle keyboard and mouse events. This returns false if a quit event is
	// received.
	bool HandleEvents();
	// Get the index of the last visible message.
	int LastVisibleIndex() const;
	// Get the maximum scroll index. This is the lowest index where the last
	// message would be fully on-screen.
	int MaxScrollIndex() const;
	// Get the index of the message displayed on the given line. If the line has
	// no message, return -1.
	int LineIndex(int line) const;
	// Get the index of the message that's a page before the given index.
	int PageBefore(int index) const;
	// Scroll up or down, always by a full page.
	void ScrollUp();
	void ScrollDown();
	
	// Parse any messages from the process.
	void ParseOutput();
	
	
private:
	// Object for handling the current child process.
	Process process;
	string buildCommand = "make";
	string cleanCommand = "make clean";
	string editCommand = "gedit FILE +LINE:COLUMN";
	bool isCleaning = false;
	
	// Parsed output:
	string title;
	vector<Message> messages;
	map<Message::Type, int> messageCount;
	vector<string> output;
	
	// Helper variables for parsing:
	string errorLocation;
	string previousError;
	int errorLinesAfter = 0;
	
	// The index of the currently selected message:
	int selectedIndex = -1;
	// The current scroll position:
	int scrollIndex = 0;
	// The size of the screen:
	int lines = 0;
	int columns = 0;
};



#endif
