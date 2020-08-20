/* Message.cpp
Michael Zahniser, 19 Dec 2018
*/

#include "Message.h"

#include <ncursesw/curses.h>

using namespace std;



// Construct a message.
Message::Message(Type type, const string &header, const string &text)
{
	// Copy the text.
	message.push_back("██ " + header);
	message.push_back(text);
	
	// Set the color.
	color = (type == ERROR ? COLOR_RED : type == WARNING ? COLOR_YELLOW : COLOR_CYAN);
	
	// Parse the text to figure out what file this error message comes from.
	// Some link errors may not list a file; they will start with '('.
	if(!text.empty() && text[0] != '(')
	{
		size_t pos = text.find(':');
		file = text.substr(0, pos);
		if(type != LINK)
		{
			line = atol(text.data() + pos + 1);
			pos = text.find(':', pos + 1);
			if(pos != string::npos)
				column = atol(text.data() + pos + 1);
		}
	}
}



// Add a line of text to the message.
void Message::AddText(const string &text)
{
	message.push_back(text);
}



// Get all the lines of text in the message.
const vector<string> &Message::Text() const
{
	return message;
}



// Get the color to use to display this message.
int Message::Color() const
{
	return color;
}



// If this message is from a particular file, get the path to the file. (If
// there is no specific file, this returns an empty string.)
const string &Message::File() const
{
	return file;
}



// If this message is linked to a particular line within a file, get that line.
// (If there is no specific line, this returns -1.)
int Message::Line() const
{
	return line;
}



// If this message is linked to a particular column within a line, get that
// column. (If there is no specific column, this returns -1.)
int Message::Column() const
{
	return column;
}
