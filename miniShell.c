#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "stdbool.h"
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>


#define ARGS_SIZE 20
#define BUFFER_SIZE 255
#define HISTORY_SIZE 100

void cwd()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\n%s> ", cwd);
}
void cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("chdir error");
        }
    }
}
void parseInput(char *input, int *numArgs, char **args, char **history)
{ 
    if (input[0] == '!')
    {
        int index = atoi(&input[1]); 
        if (index < 1 || index > HISTORY_SIZE)
        {
            printf("Invalid history index\n");
            args[0] = NULL;
            *numArgs = 0;
            return;
        }
        if (index > 0 && index <= HISTORY_SIZE)
        {
            if (history[index - 1] == NULL)
            {
                printf("No command at that index\n");
                args[0] = NULL;
                *numArgs = 0;
                return;
            }

         
            strcpy(input, history[index - 1]);
            printf("Executing command from history: %s\n", input);
        }
    }
    *numArgs = 0;
    char *tok = strtok(input, " \n");

    while (tok != NULL)
    {                         
        args[*numArgs] = tok; 
        tok = strtok(NULL, " \n");

        (*numArgs)++;
    }
    args[*numArgs] = NULL; // null terminate the array of strings
}

void IOredirect(char **args, int *numArgs, int *redirect, char **fileName)
{
    *redirect = -1; // 0 for input, 1 for output, -1 for none
    int redirectIndex = 0;
    for (int i = 0; i < *numArgs; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            // handle input redirection
            printf("Input redirection found\n");
            *redirect = 0;
            redirectIndex = i;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            // handle output redirection
            printf("Output redirection found\n");
            *redirect = 1;
            redirectIndex = i;
        }
    }
    // if redirect != -1 then there is redirection, extract everything after index
    if (*redirect != -1)
    {
        // handle IO redirection
        *fileName = args[redirectIndex + 1]; // filename is correct
        // remove everything after redirectIndex from args
        args[redirectIndex] = NULL;
        
        (*numArgs) = redirectIndex;
    }
}

void handleBackground()
{

    // reap any background processes that have finished
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // Child with PID 'pid' has finished
        printf("Background process %d finished\n", pid);
    }
}
void addHistory(char *command, char **history, int *historyCount)
{
    if (*historyCount < HISTORY_SIZE)
    {
        history[*historyCount] = strdup(command); 
        (*historyCount)++;
    }
    else
    {
        
        free(history[0]); // free memory of oldest command
        for (int i = 1; i < HISTORY_SIZE; i++)
        {
            history[i - 1] = history[i]; t
        }
        history[HISTORY_SIZE - 1] = strdup(command); 
    }
}

int main()
{
    int x = 0;
    int redirect;
    char *history[5];
    int historyCount = 0; 

    while (x == 0)
    {
        char *args[ARGS_SIZE];
        char buffer[BUFFER_SIZE];
        int numArgs = 0;
        
        int redirectIndex;
        char *fileName;
        int background = 0; // flag for background process - 0 means no, 1 means yes
        

        signal(SIGCHLD, handleBackground); // set up signal handler to reap background processes

        cwd();

        // GET INPUT IN THE MAIN:
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
        { 
            printf("NO INPUT\n");
            exit(1);
        }
        addHistory(buffer, history, &historyCount);
       
        parseInput(buffer, &numArgs, args, history);

        if (args[0] == NULL)
        {
            printf("No command entered\n");
            continue;
        }
        
        if (strcmp(args[0], "exit") == 0)
        {
            printf("Exiting\n");
            exit(0);
        }

        else if (strcmp(args[0], "history") == 0)
        {

            for (int i = 0; i < historyCount; i++)
            {
                printf("%d: %s\n", i + 1, history[i]); // i+1 to make it 1 indexed
            }
        }

        // CHECK CD
        else if (strcmp(args[0], "cd") == 0)
        {
            cd(args); // works
        }
        // CHECK FOR < or > and handle input or output redirection

        else
        {
            // flag for background process - 0 means no, 1 means yes
            if (strcmp(args[numArgs - 1], "&") == 0)
            {

                background = 1;
                args[numArgs - 1] = NULL; // remove & from args
                numArgs--;
            }
            // check for IO redirection
            IOredirect(args, &numArgs, &redirect, &fileName);

            // Fork AND EXECVP
            int pid = fork();
            if (pid == 0)
            { // child

                // handle IO redirection if needed
                if (redirect != -1)
                {

                    if (redirect == 0)
                    { // input
                        if (freopen(fileName, "r", stdin) == NULL)
                        { // redirect stdin to fileName
                            perror("Failed to redirect stdin");
                            exit(1);
                        }
                    }
                    else if (redirect == 1)
                    { // output
                        if (freopen(fileName, "w", stdout) == NULL)
                        { // redirect stdout to fileName
                            perror("Failed to redirect stdout");
                            exit(1);
                        }
                    }
                }

                if (execvp(args[0], args) == -1)
                { // execute args
                    perror("EXECVP failed");

                    fprintf(stderr, "Error code: %d\n", errno);              
                    fprintf(stderr, "Error message: %s\n", strerror(errno)); 
                }
                exit(0); // exit child process after execvp
            }
            else if (pid < 0)
            { // error
                perror("Fork failed");
                exit(1);
            }
            else
            { // parent
                if (background == 0)
                {
                    // wait for child to finish
                    int status;
                    int result = waitpid(pid, &status, 0); // blocking wait - parent waits for specific child to finish
                }
            }
        }
    }

    return 0;
}
