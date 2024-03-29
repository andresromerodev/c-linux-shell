#include "shell.h"

/* Main Function */
int main(int argc, char const *argv[]) 
{
    int isRunning = 1;
    int commandId = 1;

    char* command = malloc(sizeof(char) * MAX_CHARACTERS); 
    char** parsedCommand = malloc (sizeof(char *) * MAX_CHARACTERS); 
    char** parsedPipeCommand = malloc (sizeof(char *) * MAX_CHARACTERS); 
    char** parsedFullCommand = malloc (sizeof(char *) * MAX_CHARACTERS); 

    initShell();

    while (isRunning == 1) { 

        printf("%sshell> \x1B[1;37m",PROMPTCOLOR); // Print Shell PROMPT
        fgets(command,MAX_CHARACTERS,stdin);    // Get user input

        if (strlen(command) > 1) {

            command[strcspn(command, "\n")] = '\0'; // Remove 'ENTER'

            if (builtInHandler(command) == 0) {

                addCommandToHistory(commandId++, command); //Store command in history
                
                if (divideComposedCommand(command, parsedFullCommand) == 1) { //If a pipe is found:

                    parsedCommand = parseSingleCommand(parsedFullCommand[0]);
                    parsedPipeCommand = parseSingleCommand(parsedFullCommand[1]);
                    executeCommandWithPipe(parsedCommand, parsedPipeCommand); //Ex: cat shell.c | less

                }
                else { // If NO pipe is found:

                    parsedCommand = parseSingleCommand(parsedFullCommand[0]); 
                    executeSingleCommand(parsedCommand); //Ex: cat LICENSE & or ls -l etc...

                }
            }

            backgroundProcessFlag = 0; // Revert flag to '0'

        }
    } 

    return 0;
}


void initShell() {
    builtInCmds[0] = "exit"; 
    builtInCmds[1] = "history";
    builtInCmds[2] = "!";
    
    printf("\033[H\033[J"); // cls
    printf("\x1B[1;32mLinux Shell [Version 1.0]\n");
    printf("(c) 2019 Andres Romero. For property rights see LICENSE.\n");
    printf("GitHub Inc: https://github.com/ANDRESROMEROH\n\n");
}

// Divide command if it is compose by a pipe "|":
int divideComposedCommand(char* command, char** parsedFullCommand) 
{ 
    for (size_t i = 0; i < 2; i++) { 
        parsedFullCommand[i] = strsep(&command, "|"); 
        if (parsedFullCommand[i] == NULL) { break; }
    } 

    return (parsedFullCommand[1] == NULL) ? 0 : 1;
} 


char** parseSingleCommand(char* command) {
    char **parsedCommand = malloc (sizeof(char *) * 1024);
    char *token = malloc (sizeof(char) * MAX_CHARACTERS);
    int iterator = 0;

    token = strtok(command," \t\r\n\a");

    while(token != NULL) //Break command into single parts, Ex: ls -l -> [ls] [-l]
    {
        // If & is included process running in background:
        if(strcmp(token, "&") == 0){
            backgroundProcessFlag = 1;
            iterator ++;
        } else {
            parsedCommand[iterator++] = token;
        }

        token = strtok(NULL, " \t\r\n\a");  
    }

    parsedCommand[iterator] = NULL;

    return parsedCommand;
}


int executeSingleCommand(char** parsedCommand){
    pid_t pid;
    pid = fork();

    if (pid < 0) {
        printf("\x1B[1;31mThere was an error calling fork()\n");
        return -1;
    }
    else if (pid == 0) {
        if (execvp(parsedCommand[0], parsedCommand) == -1) {
            printf("\x1B[1;31m%s: command not found\n", parsedCommand[0]);
            exit(EXIT_SUCCESS);
        }
    }
    else {
        if(backgroundProcessFlag != 1) { // Waiting for any child process to finish...
            wait(NULL);
            wait(NULL); 
        }
    }

    return 0;
}


int executeCommandWithPipe(char** parsedCommand, char** parsedPipeCommand) 
{ 
   pid_t pid1, pid2;
   int pipefd[2];

   pipe(pipefd); // Create a pipe
   pid1 = fork(); // Create first process

   if (pid1 == 0) {
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[0]);
      execvp(parsedCommand[0], parsedCommand);

      return 1;
   }

   pid2 = fork(); // Create second process

   if (pid2 == 0) {
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[1]);
      execvp(parsedPipeCommand[0], parsedPipeCommand);
      
      return 1;
   }
   
   close(pipefd[0]); // Close reading
   close(pipefd[1]); // Close writing
   
   wait(NULL);
   wait(NULL);

   return 0;
}


void addCommandToHistory(int id, char *name)
{
    struct command *cmd;
    HASH_FIND_INT(history, &id, cmd);  // id already in the hash?

    if (cmd == NULL) {
        cmd = (struct command*)malloc(sizeof(struct command));
        cmd->id = id;
        HASH_ADD_INT(history, id, cmd);
    }

    strcpy(cmd->name, name);
}


struct command *findCommandInHistory(int id)
{
    struct command *cmd;
    HASH_FIND_INT(history, &id, cmd);

    return cmd;
}


void printHistory()
{
    int key = HASH_COUNT(history);

    for(size_t i = 0; i < HASH_COUNT(history); i++)
    {
        if (key > 0 ) {
            printCommandByID(key);
            key--;
        }
        else {
            return;
        }
    }
}


void printCommandByID(int key)
{
    struct command *cmd;

    if (key <= HASH_COUNT(history) && key > 0) {

        cmd = findCommandInHistory(key);
        printf("#%d: %s\n", cmd->id, cmd->name);

    } else {
        printf("\x1B[1;31mcommand #%d hasn't been registered\n", key);
    }

}


int builtInHandler(char* command){
    int builtInCommand = -1;

    for(size_t i = 0; i < NUM_BUILTINCMDS; i++){
        if (strcmp(command, builtInCmds[i]) == 0) { 
            builtInCommand = i; 
            break; 
        } 

        if (strpbrk(command,"!") && isdigit(command[1]) != 0) {
            command[0] = '0'; // Converting "!" will allow to parse the key using atoi()
            builtInCommand = 2; // Setup the handler to 3 which is  "!"
        }
    }

    return executeBuiltInCmd(command, builtInCommand);
}


int executeBuiltInCmd(char* command, int builtInCommand) {
    switch (builtInCommand)
    {
        case 0: // Exit
           exit(0); 

        case 1: // History
           printHistory(); 
           return 1;

        case 2: // !#
           printCommandByID(atoi(command));
           return 1;

        default:
            return 0;
    }
}