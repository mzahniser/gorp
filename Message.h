/* Message.h
Michael Zahniser, 19 Dec 2018

Class representing a single error or warning message from the compiler.
*/

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>
#include <vector>

using namespace std;



class Message {
public:
	// Message types. Each one corresponds to a different color.
	enum Type {WARNING, ERROR, LINK, NONE};
	
	
public:
	// Construct a message.
	Message(Type type, const string &header, const string &text);
	
	// Add a line of text to the message.
	void AddText(const string &text);
	
	// Get all the lines of text in the message.
	const vector<string> &Text() const;
	// Get the color to use to display this message.
	int Color() const;
	
	// Get the file and the position within that file that this message is from.
	const string &File() const;
	int Line() const;
	int Column() const;
	
	
private:
	// The text of the message, broken into lines.
	vector<string> message;
	// The color to use for displaying this message.
	int color = 0;
	
	// The file associated with this message (if any) and the position in it.
	string file;
	int line = -1;
	int column = -1;
};



#endif
