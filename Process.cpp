/* Process.cpp
Michael Zahniser, 19 Dec 2018
*/

#include "Process.h"

#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;



// Start a process by calling the given shell command.
void Process::Start(const string &command)
{
	// Clear any previously buffered output and kill any previous process.
	Kill();
	output.clear();
	errors.clear();
	
	// Copy the strings into an array of character pointers.
	Tokenize(command);
	
	// Create the pipes.
	if(pipe(outPipe) || pipe(errPipe))
		return;
	
	// For the process.
	process = fork();
	if(process < 0)
	{
		// Forking failed.
		process = 0;
		return;
	}
	else if(!process)
	{
		// We're in the child process now. Close the other end of the pipes.
		close(outPipe[0]);
		close(errPipe[0]);
		// Redirect STDOUT and STRERR into the pipes.
		dup2(outPipe[1], 1);
		dup2(errPipe[1], 2);
		close(outPipe[1]);
		close(errPipe[1]);
		
		// Launch the process.
		execvp(argv[0], &argv[0]);
	}
	else if(process > 0)
	{
		// We're in the parent process now. Close the other end of the pipes.
		close(outPipe[1]);
		close(errPipe[1]);
	}
}



// Read a line of text from STDOUT. This function will block until either the
// process has ended, or at least one line has been read from STDOUT or STDERR.
// If nothing is available from STDOUT, an empty string is returned.
string Process::ReadOutput()
{
	return Read(output);
}



// Read a line of text from STDERR. This function will block until either the
// process has ended, or at least one line has been read from STDOUT or STDERR.
// If nothing is available from STDERR, an empty string is returned.
string Process::ReadError()
{
	return Read(errors);
}



// Check if the process has finished (or has not yet been started).
bool Process::IsDone() const
{
	return !process && output.empty() && errors.empty();
}



// Tokenize a command into individual arguments, and store it in argv.
void Process::Tokenize(const string &command)
{
	// Clean up any previous arguments.
	argv.clear();
	
	// Tokenize the string.
	args = command;
	char *it = &args[0];
	while(true)
	{
		// Skip whitespace.
		while(*it && *it <= ' ')
			++it;
		// If we've reached the end of the string, bail out.
		if(!*it)
			break;
		
		// If this is a quotation mark, find the matching quote.
		char mark = *it;
		if(mark == '"' || mark == '\'')
			++it;
		else
			mark = ' ';
		
		// The token starts here.
		argv.push_back(it);
		
		// Find the end of the token.
		while(*it && *it != mark)
			++it;
		
		// If we're at the end of the string now, bail out. Otherwise, add a
		// null character to mark the end of this token, then continue parsing
		// at the next character.
		if(!*it)
			break;
		*it++ = '\0';
	}
	// Terminate the arguments vector with a null pointer.
	argv.push_back(nullptr);
}



// Extract one line of text from the given buffer. Assume that the output is
// always terminated by a newline before the EOF character.
string Process::Read(string &buffer)
{
	WaitForPipes();
	
	// See if a full line of text is contained in this buffer.
	string line;
	size_t pos = buffer.find('\n');
	if(pos != string::npos)
	{
		// Slice out this line of text and return it.
		line = buffer.substr(0, pos);
		buffer.erase(0, pos + 1);
	}
	return line;
}



// Wait until either at least a line of output has been read from one of the
// pipes, or the process has exited.
void Process::WaitForPipes()
{
	while(true)
	{
		// Check if either buffer contains a full line of text.
		if(!process || output.find('\n') != string::npos || errors.find('\n') != string::npos)
			return;
		
		// Wait until input is available in at least one of the pipes.
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(outPipe[0], &fds);
		FD_SET(errPipe[0], &fds);
		int nfds = max(outPipe[0], errPipe[0]) + 1;
		select(nfds, &fds, nullptr, nullptr, nullptr);
		
		// Read whatever output is ready.
		if(FD_ISSET(outPipe[0], &fds))
			ReadFromPipe(outPipe[0], output);
		if(FD_ISSET(errPipe[0], &fds))
			ReadFromPipe(errPipe[0], errors);
		
		// If the read returned nothing, we're at the end of the input.
		if(output.empty() && errors.empty())
		{
			CleanUp();
			return;
		}
	}
}



// Read some text from the given pipe into the given buffer.
void Process::ReadFromPipe(int fd, string &buffer)
{
	char b[1024];
	int length = read(fd, b, sizeof(b));
	if(length > 0)
		buffer.append(b, length);
}



// Kill the process (if it's still running).
void Process::Kill()
{
	if(process)
	{
		kill(process, SIGTERM);
		CleanUp();
	}
}



// Clean up the pipes, etc.
void Process::CleanUp()
{
	wait(nullptr);
	close(outPipe[0]);
	close(errPipe[0]);
	process = 0;
}
