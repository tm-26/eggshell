#define  MAX_ARGS 255
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "linenoise.h"

void errorFunction(){
    printf("\n Signal Interrupt Detected \n");
    setenv("EXITCODE", "Last process interrupted with SIGINT\n", 1);
}

int valueinarray(char array[256], char symbol){
    int n = strlen(array);

    for(int i = 0; i < n; i++){
        if(array[i] == symbol){
            return 1;

        }
    }
    return 0;
}



int main(int argc, char *argv[], char * envp[]) {

    //Initialing all variables to be used by the shell

    //int counter = -1;
    int InOut = 0;
    char *path1;
    int file = 0;
    int save_stdout = 0;
    int command_counter = 0; //Used for piping
    int fdTemp = 0;
    int fdPipe[2];
    pid_t pid1;
    int standardoutput = dup(1);
    int exit1 = 0;

    //Initialing all environmental variables.

    char *PROMPT, CWD[256], *SHELL, *TERMINAL;

    //Populating shell variables unique to eggshell.

    PROMPT = "eggshell-1.0 >";
    getcwd(CWD, sizeof(CWD));
    size_t len = sizeof(CWD);

    SHELL = malloc(len+1);
    memcpy(SHELL, CWD, len+1);
    strcat(SHELL, "/eggshell");
    TERMINAL = ttyname(STDIN_FILENO);

    setenv("CWD", CWD, 1);
    setenv("PROMPT", PROMPT, 1);
    setenv("SHELL", SHELL, 1);
    setenv("TERMINAL", TERMINAL, 1);

    //Handling Input.

    char *line, *token = NULL, *args[MAX_ARGS];

    int tokenIndex;

    while ((line = linenoise(PROMPT)) != NULL) {
        token = strtok(line, " ");
        for (tokenIndex = 0; token != NULL && tokenIndex < MAX_ARGS - 1; tokenIndex++) {
            args[tokenIndex] = token;
            token = strtok(NULL, " ");
        }

        // Handling Piping

        for (int i = 0; i < sizeof(args); i++) {
            if (args[i] == NULL) {
                break;
            }
            if (strncmp(args[i], "|", 1) == 0) {
                command_counter++;
            }
        }

        for (int loopNo = 0; loopNo <= command_counter; loopNo++) {
            if(pipe(fdPipe) < 0){
                perror("Error in piping\n");
                setenv("EXITCODE", "Error in piping\n", 1);
                exit(EXIT_FAILURE);
            }
            if(command_counter != 0) {
                pid1 = fork();
            }
            else{
                pid1 = 0;
            }

            if(pid1 < 0){
                perror("Error in forking\n");
                setenv("EXITCODE", "Error in forking\n", 1);
                exit(EXIT_FAILURE);
            }


            if(pid1 == 0) {
                if(command_counter != 0) {
                    dup2(fdTemp, STDIN_FILENO);
                    close(fdPipe[0]);

                    if (loopNo != command_counter - 1) {
                        dup2(fdPipe[1], STDOUT_FILENO);
                    }
                }

                // Handling Interrupt Management


                sighandler_t sig = signal(SIGINT, &errorFunction);


                // Handling output redirection

                int split = -1;


                for (int i = 0; i < sizeof(args); i++) {
                    if (args[i] == NULL) {
                        break;
                    }
                    if (strncmp(args[i], ">", 1) == 0) {
                        split = i;
                        break;
                    }
                }

                if (split != -1) {
                    InOut = -1;
                    path1 = malloc(len + 1);
                    memcpy(path1, CWD, len + 1);
                    strcat(path1, "/");
                    strcat(path1, args[split + 1]);
                    if (strcmp(args[split + 1], ">") != 0) {
                        fclose(fopen(path1, "w"));
                    }
                    file = open(path1, O_WRONLY | O_APPEND | O_CREAT);
                    save_stdout = dup(fileno(stdout));
                    dup2(file, 1);

                }

                //Handling here strings

                int split2 = -1;

                for (int i = 0; i < sizeof(args); i++) {
                    if (args[i] == NULL) {
                        break;
                    }
                    if (strcmp(args[i], "<<<") == 0) {
                        split2 = i;
                        break;
                    }
                }

                if (split2 != -1) {
                    if (strncmp(args[split2 + 1], "\'", 1) == 0) {
                        memmove(args[split2 + 1], args[split2 + 1] + 1, strlen(args[split2 + 1]));
                        int tracker = split2 + 2;
                        int last = 0;
                        fflush(stdin);
                        char input[256];
                        for (;;) {
                            printf(">");
                            scanf("%s", input);
                            for (int i = 0; i < strlen(input); i++) {
                                if (input[i] == '\'') {
                                    for (int j = i; j < (strlen(input) - i) - 1; j++) {
                                        input[j] = input[j + 1];
                                    }
                                    last = 1;
                                    break;
                                }
                            }

                            args[tracker] = malloc(256);
                            strcpy(args[tracker], input);
                            tracker++;
                            if (last == 1) {
                                break;
                            }
                        }
                    }
                } else {


                    //Handling Input Redirection

                    int split1 = 0;

                    for (int i = 0; i < sizeof(args); i++) {
                        if (args[i] == NULL) {
                            break;
                        }
                        if (strncmp(args[i], "<", 1) == 0) {
                            split1 = i;
                            break;
                        }
                    }


                    if (split1 != 0) {
                        FILE *fp;
                        path1 = malloc(len + 1);
                        memcpy(path1, CWD, len + 1);
                        strcat(path1, "/");
                        strcat(path1, args[split1 + 1]);
                        fp = fopen(path1, "r");
                        char str[MAX_ARGS - split1];
                        int i = split1;
                        while ((fgets(str, MAX_ARGS - split1, fp)) != NULL) {
                            memcpy(args[i], str, strlen(str) + 1);
                        }


                    }
                }


                // Handling the source function

                if (strcmp(args[0], "source") == 0) {
                    FILE *fp;
                    int i;
                    int j = 0;
                    int k = 0;
                    int length0 = 0;
                    int length1 = 0;
                    bool only_one_argument = "True";

                    char *path = malloc(len + 1);
                    memcpy(path, CWD, len + 1);
                    strcat(path, "/");
                    strcat(path, args[1]);

                    fp = fopen(path, "r");
                    if (fp) {
                        while ((i = getc(fp)) != EOF) {
                            if (i == ' ') {
                                args[j][k] = '\0';
                                if (j == 0) {
                                    length0 = k;
                                    only_one_argument = "False";
                                }
                                if (j == 1) {
                                    length1 = k;
                                }
                                j++;
                                k = 0;
                            } else {
                                args[j][k] = i;
                                k++;
                            }
                        }
                        if (only_one_argument) {
                            length0 = k;
                        }
                        if (args[0][length0] != '\0') {
                            args[0][length0] = '\0';
                        }

                        if (args[1][length1] != '\0') {
                            args[1][length1] = '\0';
                        }
                    } else {
                        printf("File does not exist\n");
                        setenv("EXITCODE", "File does not exist\n", 1);
                    }
                    fclose(fp);
                }

                // Handling inputs with '=' symbol

                if (valueinarray(args[0], '=') == 1) {
                    char variable[256];
                    char value[256];
                    char *ptr = strtok(args[0], "=");
                    strcpy(variable, ptr);
                    ptr = strtok(NULL, "=");
                    strcpy(value, ptr);

                    if (strncmp(value, "$", 1) == 0) {
                        memmove(value, value + 1, strlen(value));
                        if (getenv(value)) {
                            setenv(variable, getenv(value), 1);
                        }
                    }
                    else {
                        setenv(variable, value, 1);
                    }

                    if (strcmp(variable, "PROMPT") == 0) {
                        PROMPT = getenv("PROMPT");
                    }
                }

                    // Handling the print function

                else if (strcmp(args[0], "print") == 0) {
                    int i = 0;

                    for (;;) {
                        i++;
                        if (args[i] == '\0' || args[i] == NULL || strcmp(args[i], "(null)") == 0) {
                            break;
                        }
                        if (valueinarray(args[i], '$') == 1 && strncmp(args[i], "\\$", 2) != 0) {
                            int j = 0;
                            for (;;) {
                                if (args[i][j] == '\0' || args[i][j] == '\n') {
                                    break;
                                }

                                if (strncmp(args[i], "<<<", 3) == 0 || strncmp(args[i], "<", 1) == 0 ||
                                    strncmp(args[i], ">>", 2) == 0 || strncmp(args[i], ">", 1) == 0) {
                                    break;
                                }

                                if (strncmp(args[i], "$", 1) == 0) {
                                    memmove(args[i], args[i] + 1, strlen(args[i]));
                                    if (getenv(args[i])) {
                                        printf("%s ", getenv(args[i]));
                                        break;
                                    } else {
                                        break;
                                    }
                                } else {
                                    printf("%c", args[i][0]);
                                    memmove(args[i], args[i] + 1, strlen(args[i]));
                                }
                            }
                        } else {
                            printf("%s ", args[i]);
                            fflush(stdout);
                        }
                    }
                }

                    //Handling the chdir function

                else if (strcmp(args[0], "chdir") == 0) {
                    int n = strlen(CWD);

                    if (strcmp(args[1], "..") == 0) {
                        for (;;) {
                            if (CWD[n] != '/') {
                                CWD[n] = '\0';
                                n--;
                            } else {
                                CWD[n] = '\0';
                                setenv("CWD", CWD, 1);
                                chdir(getenv("CWD"));
                                break;
                            }
                        }
                    } else {
                        setenv("CWD", args[1], 1);
                    }
                }


                else if (strncmp(args[0], "all", 3) == 0) {
                    for (int i = 0; envp[i] != NULL; i++) {
                        printf("\n %s", envp[i]);
                    }

                    // Handling exit function

                } else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "EXIT") == 0) {
                    exit1=1;
                    exit(0);

                }

                //Handling Validation

                else if(strcmp(args[0]," ") == 0 || args[0] == NULL || strcmp(args[0],"(null)") == 0 ){
                    break;
                }

                else {
                    fflush(stdout);
                    pid_t pid = fork();
                    if (pid == -1) {
                        perror("");
                        setenv("EXITCODE", "Error in forking\n", 1);
                        exit(1);
                    } else if (pid == 0) {
                        char *arguments[MAX_ARGS];
                        char pathenv[strlen(getenv("PATH")) + sizeof("PATH=")];
                        sprintf(pathenv, "PATH=%s", getenv("PATH"));
                        char *envp1[] = {pathenv, NULL};
                        for (int i = 0; i < strlen(*args); i++) {
                            if (args[i] == NULL || strcmp(args[i], "(null)") == 0) {
                                for (int j = 0; j < strlen(*args) - i; j++) {
                                    args[i] = NULL;
                                }
                                arguments[i] = NULL;
                                break;
                            }
                        }
                        fflush(stdout);
                        args[1] = NULL;
                        if (execvpe(args[0], args, envp1)) {
                            kill(pid, SIGTERM);
                        }
                    } else {
                        int status;
                        wait(&status);
                    }
                }

                if (InOut != 0) {
                    close(file);
                    dup2(save_stdout, fileno(stdout));
                    close(save_stdout);
                    fflush(stdout);
                }
                linenoiseFree(line);

            }

            else{
                if(command_counter != 0) {
                    int current;
                    wait(&current);
                    close(fdPipe[1]);
                    fdTemp = fdPipe[0];
                }
                if (exit1 == 1) {
                    exit(0);
                }
            }
            printf("\n");

        }
    }
}
