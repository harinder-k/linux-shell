// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10

int numCommands;
char history[HISTORY_DEPTH][COMMAND_LENGTH];
char currentCommand[COMMAND_LENGTH];

void writeToScreen(char* message); //////////////////////REMOVE LATER

/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if ( (length < 0) && (errno !=EINTR) ){
	    perror("Unable to read command. Terminating.\n");
	    exit(-1);  /* terminate with error */
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	strcpy(currentCommand, buff);

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return;
	}
}


// Needed to convert command number into a readable parameter to write
size_t intToStr(char *s, int x)
{
    //  Set pointer to current position.
    char *p = s;

    //  Set t to absolute value of x.
    unsigned t = x;
    if (x < 0) t = -t;

    //  Write digits.
    do
    {
        *p++ = '0' + t % 10;
        t /= 10;
    } while (t);

    //  If x is negative, write sign.
    if (x < 0)
    {
        *p++ = '-';
    }

    //  Return values, the number of characters written.
    size_t r = p-s;

    //  Since we wrote the characters in reverse order, reverse them.
    while (s < --p)
    {
        char t = *s;
        *s++ = *p;
        *p = t;
    }

    return r;
}

void writeToScreen(char* message)
{
	write(1, message, strlen(message));
}

void addToHistory()
{
	if (numCommands <= 10)
	{
		strcpy(history[numCommands- 1], currentCommand);
	}
	else
	{
		for (int i = 0; i < HISTORY_DEPTH - 1; i++)
		{
			strcpy(history[i], history[i+1]);
		}
		strcpy(history[HISTORY_DEPTH - 1], currentCommand);
	}
}

void displayHistory()
{
	int max = (numCommands < HISTORY_DEPTH) ? numCommands : HISTORY_DEPTH;
	int commandNumOffset = (numCommands < HISTORY_DEPTH) ? 1 : numCommands - HISTORY_DEPTH + 1;
	
	for (int i = 0; i < max; i++)
	{
		int commandNum = i + commandNumOffset;
		char commandNumStr[32];
		int sizeOfCommandNumStr = intToStr(commandNumStr, commandNum);

		write(1, commandNumStr, sizeOfCommandNumStr);
		writeToScreen("\t");
		writeToScreen(history[i]);
		writeToScreen("\n");
	}
}

void retrieveCommandFromHistory(char* buf, int desiredCommandNum)
{
	if (numCommands < 10)
	{
		strcpy(buf, history[desiredCommandNum - 1]);
	}
	else
	{
		int index = HISTORY_DEPTH - 1 - (numCommands - desiredCommandNum);
		strcpy(buf, history[index]);
	}
}

/* Signal handler function */
void handle_SIGINT()
{
	writeToScreen("\n");
	displayHistory();
}

/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{

	numCommands = 0;
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];

	bool commandFromHistory = false;
	char commandBuf[COMMAND_LENGTH];		// For commands from history

	/* set up the signal handler */
	struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGINT, &handler, NULL);

	while (true) {
		bool historyCommand = false;

		// Get current working directory to display in prompt and for pwd command
		char cwdBuff[1024];
		if (getcwd(cwdBuff, sizeof(cwdBuff)) == NULL)
		{
			strcpy(cwdBuff, "Failed to retrieve current directory");
		}

		// Skipping readCommand if executing a command from history
		_Bool in_background = false;
		if (commandFromHistory)
		{
			if (commandBuf[(strlen(commandBuf) - 1)] == '&')
			{
				in_background = true;
			}
			tokenize_command(commandBuf, tokens);
			commandFromHistory = false;
		}
		else
		{
			// Get command
			// Use write because we need to use read() to work with
			// signals, and read() is incompatible with printf().
			write(STDOUT_FILENO, strcat(cwdBuff, "> "), strlen(cwdBuff) + strlen("> "));
			read_command(input_buffer, tokens, &in_background);

			// When nothing is entered
			if (input_buffer[0] == '\0')
			{
				continue;
			}
		}

		/*

		// DEBUG: Dump out arguments:
		for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		if (in_background) {
			write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
		}

		*/
/*
		//-------------------------------------------------------------------------------\\
		//------------------------------------Internal-----------------------------------\\
		//-------------------------------------------------------------------------------\\
*/
		if (strcmp(tokens[0], "exit") == 0)
		{
			exit(0);
		}

		else if (strcmp(tokens[0], "pwd") == 0)
		{
			char cwdBuff[COMMAND_LENGTH];
			if (getcwd(cwdBuff, sizeof(cwdBuff)) != NULL)
			{
				writeToScreen(strcat(cwdBuff,"\n"));
			}
			else
			{
				writeToScreen("Failed to retrieve current directory\n");
			}
		}

		else if (strcmp(tokens[0], "cd") == 0)
		{
			if (chdir(tokens[1]) == -1)
			{
				writeToScreen("Failed to change current directory\n");
			}
		}

		else if (strcmp(tokens[0], "history") == 0)
		{
			historyCommand = true;
			// must add history command to history before displaying
			numCommands++;
			addToHistory();
			displayHistory();
		}

		else if (tokens[0][0] == '!')
		{
			bool wrongLineNum = false;

			if (numCommands >= 1)
			{
				historyCommand = true;

				if (tokens[0][1] == '!')
				{
					retrieveCommandFromHistory(commandBuf, numCommands);
				}

				else
				{
					char lineNumStr[COMMAND_LENGTH];
					strncpy(lineNumStr, tokens[0] + 1, strlen(tokens[0]) - 1);

					int lineNum = atoi(lineNumStr);
					
					if (lineNum <= 0 || lineNum < numCommands - HISTORY_DEPTH || lineNum > numCommands)
					{
						wrongLineNum = true;
						writeToScreen("Invalid line number\n");
					}

					else
					{
						retrieveCommandFromHistory(commandBuf, lineNum);
					}
				}
				if (!wrongLineNum)
				{
					writeToScreen(commandBuf);
					writeToScreen("\n");
					commandFromHistory = true;
					continue;
				}
			}

			else
			{
				writeToScreen("There is no history\n");
			}
		}
/*
		//-------------------------------------------------------------------------------\\
		//-----------------------------Child process commands-----------------------------\\
		//-------------------------------------------------------------------------------\\
*/
		else
		{
			pid_t pid = fork();

		    if (pid < 0)
		    {
				writeToScreen("Failed to create child process\n");
				exit(-1);
			}
		    else if (pid == 0)
		    {
		    	if(execvp(tokens[0], tokens) == -1)
		    	{
					writeToScreen("Failed to perform command\n");
		    	}
		    	exit(0);
		    }
		    else
		    {
		    	if (!in_background)
		    	{
		    		waitpid(pid, NULL, WUNTRACED);
		    	}
		    }
		}

		// Add command to history if it's not the history command
		// 										  (because history commands get added to history separately)
		if (!historyCommand)
		{
			numCommands++;
			addToHistory();
		}

		// Cleanup any previously exited background child processes
		// (The zombies)
		while (waitpid(-1, NULL, WNOHANG) > 0)
			; // do nothing.
	}
	return 0;
}
