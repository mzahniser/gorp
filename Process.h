/* Process.h
Michael Zahniser, 19 Dec 2018

Class for launching a process and reading, line by line, anything it prints to
STDOUT or STDERR. 
*/

#ifndef PROCESS_H_
#define PROCESS_H_

#include <string>
#include <vector>

using namespace std;



class Process {
public:
	// Start a process by calling the given shell command. Strings within single
	// or double quotes are supported, but escape characters in strings are not.
	void Start(const string &command);
	
	// Read a line of text from STDOUT or STDERR. This function will block until
	// one or the other is available, or until the process has ended. If no new
	// text has been received in that particular output stream, an empty string
	// is returned. If both ReadOutput() and ReadError() return an empty string,
	// the process has stopped and all output has been returned.
	string ReadOutput();
	string ReadError();
	
	// Check if the process has finished (or has not yet been started).
	bool IsDone() const;
	
	
private:
	// Tokenize a command into individual arguments, and store it in argv.
	void Tokenize(const string &command);
	// Read a line of output from the given buffer, and return that line. The
	// returned string does not include the terminating '\n' character, if any.
	string Read(string &buffer);
	// Wait until either at least a line of output has been read from one of the
	// pipes, or the process has exited.
	void WaitForPipes();
	// Read some text from the given pipe into the given buffer.
	void ReadFromPipe(int fd, string &buffer);
	// Kill the process (if it's still running).
	void Kill();
	// Clean up the pipes, etc.
	void CleanUp();
	
	
private:
	// Arguments for the process.
	string args;
	vector<char *> argv;
	// Process ID.
	int process = 0;
	// Pipes for communication with the process.
	int outPipe[2] = {-1, -1};
	int errPipe[2] = {-1, -1};
	// Output and errors received and queued up.
	string output;
	string errors;
};



#endif
