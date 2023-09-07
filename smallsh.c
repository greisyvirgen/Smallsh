// Greisy Virgen Larios 
// Credit to the professor and the exploration. The code examples provided and 
// structure of the code was used as baseline to create certain functions here. 

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);

// GLOBALS for further use
char *money_excl = "";
char *money_q = "0";
int background = 1; // ig 0 it means its true, background present
char *infile = NULL; 
char *outfile = NULL;

// Signal handling
void handle_SIGINT(int signo){
}

// Check for any un-waited-for backgroung processes in same process group ID
int main(void)
{
  
  // exit al unwaited for backgroung process
  //char *arr_words[512];
  char* line = NULL;
  size_t len = 0;


the_loop:
  for(;;) { 
    char *arr_words[512];

    int child_stat;
    pid_t child_bg; 

    while ((child_bg = waitpid(0, &child_stat, WUNTRACED | WNOHANG)) > 0) {

      if (WIFEXITED(child_stat)){
        int exit_stat = WEXITSTATUS(child_stat);
        
        fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t) child_bg, exit_stat);
      }
      else if (WIFSIGNALED(child_stat)){
        int e_stat = WTERMSIG(child_stat);
        
        fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) child_bg, e_stat);
      }
      else if (WIFSTOPPED(child_stat)){
        kill(child_bg, SIGCONT);
        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) child_bg);
      }
      
    } // while loop end


    // Printing the PS1 prompt accordingly
    char *check_env;
    check_env = getenv("PS1");

    if (check_env == NULL) {
      fprintf(stderr, "%s", "");
    } else {
      fprintf(stderr, "%s", check_env);
    }
    
    // signal handling
    struct sigaction SIGINT_dummy = {0}, ignore = {0}, old = {0};

    // To ignore
    ignore.sa_handler = SIG_IGN;

    // To register the ifnore ones
    sigaction(SIGTSTP, &ignore, NULL);
    sigaction(SIGINT, &ignore, NULL);


    // Now do SIGINT
    SIGINT_dummy.sa_handler = handle_SIGINT;
    sigfillset(&SIGINT_dummy.sa_mask);
    SIGINT_dummy.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_dummy, NULL);
    

    // Getting the line and setting errno to 0 before starting 
    errno = 0;
    ssize_t line_read = getline(&line, &len, stdin);
    

    if (feof(stdin)){
        fprintf(stderr, "\nexit\n");
        int stat = WEXITSTATUS(child_stat);
        exit(stat);
    }

    if (strcmp(line, "") == 0){
      putchar('\n');
      free(line);
      line = NULL;
      len = 0;
      errno = 0;
      goto the_loop;
    }

    if (line_read == -1 && errno == EINTR){
      // We print a new line and reset errno as need, 
      // then we can move on to print the prompt again.
      putchar('\n');
      clearerr(stdin);
      free(line);
      line = NULL;
      len = 0;
      errno = 0;
      goto the_loop;
    }
    // Reset after used for getline.
    sigaction(SIGINT, &ignore, NULL);

    // Word splititng to set IFS
   // word_splitting();

    char *the_IFS;
    the_IFS = getenv("IFS");
    //char *token;
    if (the_IFS == NULL){
      the_IFS = " \t\n";
    } else {
      the_IFS = getenv("IFS");
    }

    char *token =  strtok(line, the_IFS);
    int ctr = 0; // counter to add to array
                 
    while (token != NULL) {
      arr_words[ctr] = strdup(token);
      token = strtok(NULL, the_IFS);
      ctr ++;
      
    }
    // Null terminating my array 
    arr_words[ctr] = NULL;
    
    /*for (int x = 0; x <512; x++){
      if (arr_words[x] == NULL) {
        break;
      }
      printf("%s", arr_words[x]);
    }*/
    
    // Expansion: Using the information provided on the video by the Professor
  
    char *the_home;
    the_home = getenv("HOME");
    
    if (the_home == NULL){
      the_home = "";
    } else {
      the_home = getenv("HOME");
    }
                            
    // Getting the pid
    pid_t pid = getpid();
    char pid_str[8]; // unsure of what size to make it (saw 8 on discord chat)
    sprintf(pid_str, "%d", pid);  // Now pid is in string form

    // Now I check my actual array
    for (int i = 0; i < ctr; i++) {
      if (strncmp(arr_words[i], "~/", 2) == 0) {
        arr_words[i] = str_gsub(&arr_words[i], "~", the_home);

      } 
      arr_words[i] = str_gsub(&arr_words[i], "$$", pid_str); 
      arr_words[i] = str_gsub(&arr_words[i], "$?", money_q);
      arr_words[i] = str_gsub(&arr_words[i], "$!", money_excl);
    }   
   
    //new parsing loop
    for (int i = 0; i < ctr; i++) {
      if (strcmp(arr_words[i], "<") == 0) {
        free(arr_words[i]);
        arr_words[i] = NULL;
        infile = arr_words[i + 1];
      } else if (strcmp(arr_words[i], ">") == 0) {
        free(arr_words[i]);
        arr_words[i] = NULL;
        outfile = arr_words[i + 1];
      } else if (strcmp(arr_words[i], "&") == 0) {
        background = 0; // now its set
        free(arr_words[i]);
        arr_words[i] = NULL;
      } else if (strcmp(arr_words[i], "#") == 0) {
        free(arr_words[i]);
        arr_words[i] = NULL;
      }
    }
     
    // Execution
    errno = 0;
    if (arr_words[0] == NULL) {
      free(line);
      line = NULL;
      //putchar('\n');
      errno = 0; 
      len = 0;
      goto the_loop;

      // CD command
    } else if (strcmp(arr_words[0], "cd") == 0) {
      if (arr_words[1] == NULL) {
        chdir(getenv("HOME"));

      } else if( arr_words[2]) {
        fprintf(stderr, "Too many arguments, invalid.\n");
        exit(errno);
        errno = 0;

      }else {
        if (chdir(arr_words[1]) == -1) {
            fprintf(stderr, "Could not find directory.\n");
            errno = 0;
            goto the_loop;
        } else { 
          chdir(arr_words[1]); 
        }
      }
    }
      // Exit command
    else if (strcmp(arr_words[0], "exit") == 0) {

      if (arr_words[1] == NULL) {
        fprintf(stderr, "\nexit\n");
        int stat = WEXITSTATUS(child_stat);
        exit(stat);
      }

      else if (arr_words[2]) { // too many arguments
       fprintf(stderr, "Too many arguments, invalid.\n");
       exit(errno);
       errno = 0;
      // goto the_loop;

      } else if (arr_words[1] != NULL) {
        char int_val = 1; //meaning false
        int exit_val;
        //int len = strlen(arr_words[1]);
        for (size_t i = 0; i < sizeof(arr_words[1]); i++) {
          if (isdigit(arr_words[1][i]) != 0) {
            int_val = 0;
          }
            //continue;
          /*} else { int_val = 1;}*/
        }
        if (int_val == 0) {
          exit_val = atoi(arr_words[1]);
          fprintf(stderr, "\nexit\n");
          exit(exit_val);
        }
        else {
          fprintf(stderr, "Arguments should be integers.\n");
          exit(errno);
          errno = 0;
        }

      } /*else {
          fprintf(stderr, "\nexit\n");
          //int exit_s = WEXITSTATUS(money_q);
          exit(*money_q);
        } */
      

    // Looking at nonbuilt in commands
    } else {
      //if (arr_words[0] == NULL) {
        //goto the_loop;
      //}

      // FORK PROCESS -- NEED TO ALTER
      pid_t spawnpid;
      //int intVal = 10;

      int child_stat;
      spawnpid = fork();
      switch (spawnpid) {
        case -1:
          fprintf(stderr, "Call to fork() failed!\n");
          exit(1);
          break;
        
        case 0:
          // The CHILD
          errno = 0;

          // To open: 0 is stdin
          // 1 is stdout, 
          // 2 is stderr
          // The code below was adapted from the exploration Processes and I/O

          if (infile != NULL) {
            // Then try to open the file
            // < means infile, open for reading on STDIN
            // error if can't open or does not exist
            int source_file = open(infile, O_RDONLY);

            // Check if it can't be opened
            if (source_file == -1) {
              fprintf(stderr, "File could not be opened\n");
              exit(1);
            }
            int opened_source = dup2(source_file, 0);

            if (opened_source == -1) {
              fprintf(stderr, "Could not open with dup2\n");
              exit(2);
            }
          }

          if (outfile != NULL) {
            int target_file = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);

            if (target_file == -1) {
              fprintf(stderr, "File could not be opened or changed\n");
              exit(1);
            }
            // to stdout
            int result = dup2(target_file, 1);
           
            if (result == -1) {
              fprintf(stderr, "Could not open with dup2\n");
              exit(2);
            }
          }
                   
         /* // Debugging code
          fprintf(stderr, "\nWord Array !!!!\n");
          fprintf(stderr, "%d\n", ctr);
          fprintf(stderr, "\n%s\n", arr_words[0]); // my command
          for (int i =0; i <ctr; i++) {
            fprintf(stderr, "%s", arr_words[i]);
          }
          fprintf(stderr,"\nEND OF ARRAY CHECK BEFORE EXECVP\n");
        */
        
          sigaction(SIGINT, &old, NULL);
          sigaction(SIGTSTP, &old, NULL);

          execvp(arr_words[0], arr_words);
          fprintf(stderr, "execvp has failed\n");
          exit(EXIT_FAILURE);
          break;
          
        default: // This is the parent

          // NOW check if background
          if (background == 0) {
            money_excl = malloc(8);
            sprintf(money_excl, "%d", spawnpid);
          }

          else{
            // Means no background established

            //perform bloicking wait on foreground child process
            waitpid(spawnpid, &child_stat, 0);
            
            // e_status = WEXITSTATUS(child_stat);
            if (WEXITSTATUS(child_stat)) {
              int ex_stat = WEXITSTATUS(child_stat);
              money_q = malloc(8);
              sprintf(money_q, "%d", ex_stat);
            }

            if (WIFSIGNALED(child_stat)) {
              int sig_stat = WTERMSIG(child_stat);
              sig_stat += 128;
              money_q = malloc(8);
              sprintf(money_q, "%d", sig_stat);
                  
            }

            if (WIFSTOPPED(child_stat)) {
              int child_p = waitpid(spawnpid, &child_stat, WUNTRACED | WNOHANG);
              kill(child_stat, SIGCONT);
              fprintf(stderr, "Child process %d stopped. Continuing.\n", child_p);
              money_excl = malloc(8);
              sprintf(money_excl, "%d", child_p);
            }
          }
          break;

      } // switch end
    }
   

    free(line);
    line = NULL;
    //free(infile);
    infile = NULL;
    //free(outfile);
    outfile = NULL;
    background = 1;
    len = 0;
   
    for (int i = 0; i < ctr; i++){
      free(arr_words[i]);
      arr_words[i] = NULL;
    }
  }

return 0;
}



// Portion provided by the video on from the Professor
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub)
{
  char *str = *haystack;
  size_t haystack_len = strlen(str);
  size_t const needle_len = strlen(needle), 
               sub_len = strlen(sub);

  for (; (str = strstr(str, needle));) {
    ptrdiff_t off = str - *haystack;
    if (sub_len > needle_len) {
      str = realloc(*haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1));
      if (!str) {
        goto exit;
      }
      *haystack = str;
      str = *haystack + off;

    }

    memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
    memcpy(str, sub, sub_len);
    haystack_len = haystack_len + sub_len - needle_len;
    str += sub_len;
    
    // MIGHT NEED TO MOVE TO MAIN
    if(strcmp(needle, "~") == 0) {
      break;
    }
  }

  str = *haystack;
  if (sub_len < needle_len) {
    str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
    if (!str) {
      goto exit;
    }
    *haystack = str;
  }
  
  
exit:
  return str;
}

