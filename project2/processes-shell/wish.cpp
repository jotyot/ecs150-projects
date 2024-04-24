#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>

#include <string>
#include <string.h>
#include <vector>
#include <algorithm>

using namespace std;

struct command
{
    string outFile;
    vector<string> commandArgs;
};

void wish(char *file);
vector<command> parseCommand(string inputString);
vector<string> splitAmpersand(string command);
vector<string> splitSpaces(string command);
char **convertToChars(vector<string> commandArgs);
bool builtInCommands(command cmd, vector<string> *path);

int main(int argc, char *argv[])
{
    if (argc == 1)
        wish(NULL);
    else if (argc == 2)
        wish(argv[1]);
    else
    {
        cerr << "An error has occurred" << endl;
        return 1;
    }
    return 0;
}

void wish(char *file)
{
    vector<string> path = {"/bin"};

    // if file is NULL, then we are in interactive mode
    // if interative, read from cin, otherwise from file
    bool interactive = file == NULL;
    istream *inputStream;
    ifstream fileStream(file);
    if (interactive)
    {
        inputStream = &cin;
        cout << "wish> ";
    }
    else
    {
        inputStream = &fileStream;
        if (!fileStream.is_open())
        {
            cerr << "An error has occurred" << endl;
            exit(1);
        }
    }

    // read input line by line
    string inputString;
    while (getline(*inputStream, inputString))
    {
        vector<command> commands = parseCommand(inputString);
        vector<pid_t> pids;
        for (command cmd : commands)
        {
            // built in command check
            if (builtInCommands(cmd, &path))
                continue;
            char **args = convertToChars(cmd.commandArgs);
            pid_t pid = fork();
            pids.push_back(pid);
            if (pid == 0)
            {
                // redirect output to file if specified
                if (!cmd.outFile.empty())
                {
                    int fd = open(cmd.outFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                // try paths until one executes
                for (const string &pathDir : path)
                {
                    const char *pathCommand = (pathDir + "/" + args[0]).c_str();
                    if (access(pathCommand, X_OK) == 0)
                    {
                        execv(pathCommand, args);
                    };
                }
                cerr << "An error has occurred" << endl;
                exit(1);
            }
        }
        // wait for all concurrent processes to finish before reading next line
        for (pid_t pid : pids)
        {
            waitpid(pid, NULL, 0);
        }

        if (interactive)
            cout << "wish> ";
    }
}

// tests for empty command, exit, cd, and path commands
// returns true if no need to fork
bool builtInCommands(command cmd, vector<string> *path)
{
    if (cmd.commandArgs.empty())
        return true;

    if (cmd.commandArgs[0] == "exit")
    {
        if (cmd.commandArgs.size() != 1)
            cerr << "An error has occurred" << endl;

        exit(0);
    }
    if (cmd.commandArgs[0] == "cd")
    {
        if (cmd.commandArgs.size() != 2)
            cerr << "An error has occurred" << endl;

        else
        {
            if (chdir(cmd.commandArgs[1].c_str()) < 0)
                cerr << "An error has occurred" << endl;
        }
        return true;
    }
    if (cmd.commandArgs[0] == "path")
    {
        path->clear();
        for (const string &arg : cmd.commandArgs)
        {
            path->push_back(arg);
        }
        return true;
    }
    return false;
}

// takes a string and parses it into a vector of commands
// each command is a struct with a vector of strings for the command arguments
// and a string for the output file
vector<command> parseCommand(string inputString)
{
    vector<command> commands;
    vector<string> commandStrings = splitAmpersand(inputString);
    for (string commandString : commandStrings)
    {
        command newCommand;

        // check for output redirection, set output file member if found
        size_t redirectPos;
        if ((redirectPos = commandString.find(">")) != string::npos)
        {
            string rhs = commandString.substr(redirectPos + 1);
            vector<string> outFiles = splitSpaces(rhs);
            if (outFiles.size() != 1)
            {
                cerr << "An error has occurred" << endl;
                continue;
            }
            string outFile = outFiles[0];
            commandString = commandString.substr(0, redirectPos);
            if (commandString.empty())
            {
                cerr << "An error has occurred" << endl;
                continue;
            }
            newCommand.outFile = outFile;
        }
        vector<string> commandArgs = splitSpaces(commandString);
        newCommand.commandArgs = commandArgs;
        commands.push_back(newCommand);
    }
    return commands;
}

// takes a string and splits into seperate strings whenever there is an &
vector<string> splitAmpersand(string command)
{
    vector<string> commands;
    size_t pos = 0;
    string token;
    while ((pos = command.find("&")) != string::npos)
    {
        token = command.substr(0, pos);
        commands.push_back(token);
        command.erase(0, pos + 1);
    }
    commands.push_back(command);
    return commands;
}

// takes a string and splits into seperate strings whenever there is a space
// no matter how many spaces are between the strings
vector<string> splitSpaces(string command)
{
    vector<string> commands;
    size_t pos = 0;
    string token;
    while ((pos = command.find(" ")) != string::npos)
    {
        token = command.substr(0, pos);

        if (!token.empty())
            commands.push_back(token);

        command.erase(0, pos + 1);
    }
    if (!command.empty())
        commands.push_back(command);
    return commands;
}

// takes a vector of strings and converts it to a char** for execv
char **convertToChars(vector<string> commandArgs)
{
    char **args = new char *[commandArgs.size() + 1];
    for (size_t i = 0; i < commandArgs.size(); i++)
    {
        args[i] = new char[commandArgs[i].size() + 1];
        strcpy(args[i], commandArgs[i].c_str());
    }
    args[commandArgs.size()] = NULL;
    return args;
}