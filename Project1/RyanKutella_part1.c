//This project creating Unix terminal using kernel commands.

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 80 /* The maximum length command */

int main(void)
{
    char *args[MAX_LINE / 2 + 1]; /* command line arguments */
    int should_run = 1;           /* flag to determine when to exit program */

    while (should_run)
    {
        printf("Kutella_%d>",getpid());
        fflush(stdout);

        char inputs[1000];

        fgets(inputs,sizeof(inputs),stdin); //Is going to get the entire input as is
        inputs[strlen(inputs)-1] = '\0';    //Just put a stopper at end of inputs

        int counter = 0;    //Counter to go through all inputs (words)
        int hasAnd = 0;     //"Boolean" like int (0 means no &, 1 means has &)
        char *eachWord = strtok(inputs," ");    //Takes input up to first space so the first word

        while(eachWord != NULL){ //Go until no more words
            // printf("%d %s\n",counter,eachWord);

            if(!strcmp(eachWord,"&")){  
                hasAnd = 1;     //if has an & set int to 1;
            }
            else if(!strcmp(eachWord,"exit")){  
                return 0;
            }
            else{
                args[counter] = eachWord;   //In else because don't want to add & to command
            }

            counter++; 
            eachWord = strtok(NULL," ");    //go to next "word" in input
        }
        args[counter] = NULL;       //Sets end of args to a stopper value 

        pid_t pid;
        pid = fork();  //gets new child with new pid

        if(pid < 0){
            perror("Fork Process Failed\n"); //pid should never be less than 0
            return 1;
        }
        else if(pid==0){ //Child proccesses
            if(execvp(args[0],args)==-1){
                printf("Command does not exist or failed.\n"); //Should not get hear unless execvp calls error
            }
        }
        else{
            if(!hasAnd){
                //printf("Went thru\n"); //just for testing
                while(wait(NULL)!=pid);  //will wait when there was no &
            }
        }

        fflush(stdout);

    }

    return 0;
}

