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

int interactive();
void batch(char *file);
vector<command> parseCommand(string inputString);
vector<string> splitAmpersand(string command);
vector<string> splitSpaces(string command);
string removeSpaces(string str);
char **convertStringVector(vector<string> commandArgs);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        return interactive();
    }
    else if (argc == 2)
    {
        batch(argv[1]);
    }
    else
    {
        cout << "Usage: wish [batch-file]" << endl;
        return 1;
    }
    return 0;
}

int interactive()
{
    string path = "/bin";
    string inputString;
    while (true)
    {
        cout << "wish> ";
        getline(cin, inputString);
        if (inputString == "exit")
        {
            return 0;
        }
        vector<command> commands = parseCommand(inputString);
        for (command cmd : commands)
        {
            char **args = convertStringVector(cmd.commandArgs);
            pid_t pid = fork();
            if (pid == 0)
            {
                if (!cmd.outFile.empty())
                {
                    int fd = open(cmd.outFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                execv((path + "/" + args[0]).c_str(), args);
                cerr << "An error has occurred" << endl;
                exit(1);
            }
            else
            {
                wait(NULL);
            }
        }
    }

    return 0;
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
        size_t redirectPos;
        if ((redirectPos = commandString.find(">")) != string::npos)
        {
            string outFile = commandString.substr(redirectPos + 1);
            commandString = commandString.substr(0, redirectPos);
            outFile = removeSpaces(outFile);
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
vector<string> splitSpaces(string command)
{
    vector<string> commands;
    size_t first = 0;
    string token;
    while ((first = command.find_first_not_of(' ')) != string::npos)
    {
        size_t last = command.find(' ', first);
        token = command.substr(first, last);
        commands.push_back(token);
        command.erase(0, last + 1);
        if (token == command)
        {
            break;
        }
    }
    return commands;
}

string removeSpaces(string str)
{
    str.erase(remove(str.begin(), str.end(), ' '), str.end());
    return str;
}

char **convertStringVector(vector<string> commandArgs)
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

void batch(char *file)
{
}
