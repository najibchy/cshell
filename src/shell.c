#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>


#define LSH_RL_BUFSIZE 1024 //Initial size of line buffer size
#define LSH_TOK_BUFSIZE 64 //Initial size of token buffer size
#define LSH_TOKEN_DELIM " \t\r\n\a"

/* 
Functions Declarations
*/
int lsh_cd(char ** args);
int lsh_help(char ** args);
int lsh_exit(char ** args);

/* 
List of commands
*/
char *builtin_str[] =  {
    "cd",
    "help",
    "exit"
};

/* 
List of functions
*/
int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

// Determine number of builtin functions
int lsh_num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char ** args){
    // Show error if the second argument is NULL
    if( args[1] == NULL){
        fprintf(stderr, "lsh: error");
    }
    else {
        //Change the directory to the second argument
        if (chdir(args[1]) != 0){
            perror("lsh");
        }
    }
    return 1;
}

int lsh_exit(char ** args){
    //Exit the program
    return 0;
};

int lsh_help(char ** args){
    printf("Najib Chowdhury's Shell\n");
    printf("Type the function name and arguments\n");

    // Loops to determine all the builtin functions
    for(int i = 0; i < lsh_num_builtins(); i++){
        printf("%d: %s\n", i, builtin_str[i]); 
    }

    printf("Use the command for information");
    return 1;
}

int lsh_launch(char **args) {
    /* Windows API for creating new process
    https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa 
    It is equivalent to fork() in Linux*/
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD status;  // Change status to DWORD

    // Initialize memory for the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Convert the arguments into a single string for Windows
    char commandLine[1024] = "";
    for (int i = 0; args[i] != NULL; i++) {
        strcat(commandLine, args[i]);
        strcat(commandLine, " ");
    }

    // Attempt to create the new process
    if (!CreateProcess(
            NULL,            // Application name (NULL because we include it in command line)
            commandLine,     // Command line (application + arguments)
            NULL,            // Process security attributes
            NULL,            // Thread security attributes
            FALSE,           // Inherit handles
            0,               // Creation flags (0 for default)
            NULL,            // Use parent's environment
            NULL,            // Use parent's current directory
            &si,             // Pointer to STARTUPINFO
            &pi              // Pointer to PROCESS_INFORMATION
        )) {
        // Error creating process
        printf("Error creating process (%d)\n", GetLastError());
        return -1;
    }

    // Wait until child process exits
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Get the exit status of the process
    if (!GetExitCodeProcess(pi.hProcess, &status)) {
        printf("Error getting exit code (%d)\n", GetLastError());
        return -1;
    }

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return status;
}

int lsh_execute(char ** args){
    printf("Executing command: %s\n", args[0]);
    if (args[0] == NULL){
        return 1;
    }
    for(int i = 0; i < lsh_num_builtins(); i++){
        if(strcmp(args[0], builtin_str[i]) == 0){
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
}

char *lsh_read_line(void){
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char)*bufsize);
    int c;

    if(!buffer){
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        // Read the character
        c = getchar();
        // Replace EOF(End of File) and newlines with NULL character
        if (c == EOF || c == '\n'){
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;
        //Reallocate memory if buffer size is exceeded
        if (position >= bufsize){
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer){
                // Show error if reallocation fails
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **lsh_split_line(char *line){
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*)); //Allocate memory for tokens
    char *token;

    if (!tokens){
        //Show error if reallocation fails
        fprintf(stderr,"lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, LSH_TOKEN_DELIM); //Split the prompt into tokens
    while (token != NULL){
        tokens[position] = token;
        position++;

        if (position >= bufsize){
            bufsize += LSH_TOK_BUFSIZE; //See if memory is exceeded
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens){
                fprintf(stderr, "error");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOKEN_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void lsh_loop(void){
    //Read, parse and execute the input
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv){
    //Load the configuration files

    //Run command loop
    lsh_loop();

    return EXIT_SUCCESS;
}
