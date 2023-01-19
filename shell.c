#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */
#define MAX_ARGS 40 //max length args
#define HIST_PATH ".history" //history path file

int waitTime;
int inFile; //inputs to direct to command
int outFile; //output to direct to file
int pipeCom; //pipe communication
int in, savedIn; //in file and saved in file
int out, savedOut; //out file and saved out file
int saveComm; //saved commands

void toToken(char *command, char **args) {
  int argCounter = 0; //argument counter
  int commLength = strlen(command); //command length
  int argStart = -1; //arg starting input

  for (int i = 0; i < commLength; i++) {
    //check if token is empty, new line, or tab
    if (command[i] == ' ' || command[i] == '\n' || command[i] == '\t') {
      //check if starting input is in the middle of the command
      //then "restart" the command
      if (argStart != -1) {
        args[argCounter++] = &command[argStart];
        argStart = -1;
      };

      command[i] = '\0'; //token will be null
    }
    else {

      //check if command has '&' to run concurrently
      if (command[i] == '&') {
        waitTime = 0;
        i = commLength; //end of loop
      };

      //check if starting in middle of command
      if (argStart == -1)
      {
        argStart = i;
      };
    };
  };

  args[argCounter] = NULL; //complete tokenization
};

//history feature
void historyFeat(char **args) {
  //open history file, read mode
  FILE *hist = fopen(HIST_PATH, "r");

  if (hist == NULL) {
    printf("Sorry! History is empty. \n");
  }
  else {
    if (args[1] == NULL) {
      //get input single char at a time
      char c = fgetc(hist);
      while (c != EOF) {
        printf("%c", c);
        c = fgetc(hist);
      };// while (comm != EOF); //EOF is end of file
    }
    else if (!strcmp(args[1], "-c")) {
      saveComm = 0;
      remove(HIST_PATH);
    }
    else {
      printf("Sorry! Invalid synatax. Please try again.");
    };

    fclose(hist);
  };
};


//record commands in history, like a helper function almost
void historyRecorder(char *command) {
  FILE *hist = fopen(HIST_PATH, "a+"); //a+ to create new file
  fprintf(hist, "%s", command); //print command to file
  rewind(hist); //restart to beginning of file
};

//redirecting input and output
void flagCheck(char **args) {
  for (int i = 0; args[i] != NULL; i++) {
    //redirect output of command to file
    if (!strcmp(args[i], ">")) {
      if (args[i + 1] == NULL) {
        printf("Sorry! Invalid command format. Please try again.");
      }
      else {
        outFile = i + 1;
      };
    };

    //redirect input to command from file
    if (!strcmp(args[i], "<")) {
      if (args[i + 1] == NULL) {
        printf("Sorry! Invalid command format. Please try again.");
      }
      else {
        inFile = i + 1;
      };
    };

    //communication via pipe
    if (!strcmp(args[i], "|")) {
      if (args[i + 1] == NULL) {
        printf("Sorry! Invalid command after | . Please try again.");
      }
      else {
        pipeCom = i;
      };
    };
  };
};

//execute helper
void exec(char **args) {
  if (execvp(args[0], args) < 0) {
    printf("Sorry! Invalid command.\n");
    exit(1);
  };
};

int main(void)
{
  //char *args[MAX_LINE/2 + 1]; /* command line arguments */
  int should_run = 1; /* flag to determine when to exit program */

  char command[MAX_LINE]; //array of commands to execute
  char tokenCommand[MAX_LINE]; //tokenized commands
  char lastCommand[MAX_LINE]; //last command user inputted
  char *args[MAX_ARGS];
  char *argptr1[MAX_ARGS]; //argument pointer 1
  char *argptr2[MAX_ARGS]; //argument pointer 2
  int history = 0;
  int alerts;
  int pipeCH[2]; //pipe channel, just pipe[2] confuses compiler

  //char execOrder[MAX_LINE]; //record ';', '&', and '|' to execute properly
  
  /*
  enum{READ, WRITE};
  pid_t pid; //compiler doesn't like pit_t find out why
  int pipeFD[2];
  if (pipe(pipeFD) < 0) {
    perror("Error: pipe failed.");
    //exit();
  };*/

  while (should_run) {
    printf("osh>");
    fflush(stdout);
    /**
    * After reading user input, the steps are:
    * (1) fork a child process using fork()
    * (2) the child process will invoke execvp()
    * (3) parent will invoke wait() unless command included &
    */

    fgets(command, MAX_LINE, stdin);
    
    waitTime = 1;
    alerts = 0;
    outFile = -1;
    inFile = -1;
    pipeCom = -1;
    saveComm = 1;

    strcpy(tokenCommand, command);
    toToken(tokenCommand, args);
    
    /*
    //check if command includes ';', '&', or '|'
    if (strcmp(args[0], ";") || strcmp(args[0], "&") || strcmp(args[0], "|")) {
      
      exec(args);

    };*/

    //check if args is empty or is a new line
    if (args[0] == NULL || !strcmp(args[0], "\0") || !strcmp(args[0], "\n")) {
      continue;
    };

    //check if user wants to exit program
    if (!strcmp(args[0], "exit")) {
      should_run = 0;
      continue;
    };

    //check if user wants to see history
    if (!strcmp(args[0], "!!")) {
      if (history) {
        printf("%s", lastCommand);
        strcpy(command, lastCommand);
        strcpy(tokenCommand, command);
        toToken(tokenCommand, args);
      }
      else {
        printf("Sorry! No commands in history.");
        continue;
      };
    };

    /*
    //attempted bonus for ascii
    if (!strcmp(args[0], "ascii")) {
      printf("  (|_|)\n");
      printf(" (='.'=)\n");
      printf("( > º < )\n");
      printf("('')_('')\n");
      continue;
    };*/

    flagCheck(args);

    //open in file
    if (inFile != -1) {
      //flags: read only, create file if not existing
      //0777 is for permissions
      in = open(args[inFile], O_RDONLY);// | O_CREAT, 0777);
      
      if (in < 0) {
        printf("Sorry! File failed to open. %s \n", args[inFile]);
        alerts = 1;
      }
      else {
        savedIn = dup(0); //duplicate file to in
        dup2(in, 0);
        close(in);
        args[inFile - 1] = NULL;
      };
    };

    //open out file
    if (outFile != -1) {
      //flags: write only, create file if not existing
      //0777 is for permissions
      out = open(args[outFile], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      //| O_CREAT, 0777);
      
      if (out < 0) {
        printf("Sorry! File failed to open. %s \n", args[outFile]);
        alerts = 1;
      }
      else {
        savedOut = dup(1); //duplicate file to in
        dup2(out, 1);
        close(out);
        args[outFile - 1] = NULL;
      };
    };

    //pipe communication establishment
    if (pipeCom != -1) {
      int i = 0;
      for (; i < pipeCom; i++) {
        argptr1[i] = args[i];
      };
      argptr1[i] = NULL;
      
      i++;

      for (; args[i] != NULL; i++) {
        argptr2[i - pipeCom - 1] = args[i];
      };
      argptr2[i] = NULL;
    };

    if (!alerts && should_run) {
      if (!strcmp(args[0], "history")) {
        historyFeat(args);
      }
      else {
        //go back to this and attempt to revise?
        if (!strcmp(args[0], "stop") || !strcmp(args[0], "continue")) {
          args[2] = args[1];
          if (strcmp(args[0], "stop")) {
            args[1] = "-SIGCONT";
          }
          else {
            args[1] = "-SIGSTOP";
          };
          args[0] = "kill";
          args[3] = NULL;
        };

        if (fork() == 0) { //in child process
          if (pipeCom != -1) {
            pipe(pipeCH);

            if (fork() == 0) { //in child of child process
              savedOut = dup(1);
              dup2(pipeCH[1], 1);
              close(pipeCH[0]);
              exec(argptr1);
            }
            else { //in parent of child process
              wait(NULL);
              savedIn = dup(0);
              dup2(pipeCH[0], 0);
              close(pipeCH[1]);
              exec(argptr2);
            };
          }
          else { //execute args as is
            exec(args);
          };
        }
        else { //in parent process
          if (waitTime) {
            wait(NULL);
            //continue;
          };
        };
      };

      strcpy(lastCommand, command);

      if (saveComm) {
        historyRecorder(command);
        history = 1;
      };
    };

    dup2(savedOut, 1);
    dup2(savedIn, 0);
    /*
    //fork child process using fork()
    //pid_t pid;
    pid = fork();
    if (pid < 0) {
      printf(stderr, "Error: fork failed");
      //exit();
    }
    else if (pid == 0) {//in child process
      close(pipeFD[READ]);
      dup2(pipeFD[READ], NULL);
      printf("Child start.");
      //child invokes execvp()
      //execvp(char *command, char* parameters[]);
      printf("Child end.");
    }
    else { //in parent process
      close(pipeFD[WRITE]);
      dup2(pipeFD[READ], 0); //double-check the int value
      printf("Parent start.");
      wait(NULL);
      printf("Parent end.");
    };
    */
  };
  
  return 0;
};