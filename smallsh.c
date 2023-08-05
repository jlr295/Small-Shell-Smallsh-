#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
size_t wordsplit(char const *line);
char * expand(char const *word);
sigjmp_buf environment;

struct process

{ pid_t foregroundpid;
  int foregroundexstatus;
  pid_t backgroundpid;
};

struct process startprocess;

void handle_SIGINT(int signo){
  fprintf(stderr, "\n");
  siglongjmp(environment, 1);
}


int main(int argc, char *argv[])
{

struct sigaction ignore_action = {0}, SIGINT_action = {0}, previousHandlerSIGINT = {0}, 
                 previousHandlerSIGTSTP = {0};

SIGINT_action.sa_handler = handle_SIGINT;
SIGINT_action.sa_flags = SA_RESTART;
ignore_action.sa_handler = SIG_IGN;
sigsetjmp(environment, 1);

  FILE *input = stdin;
  char *input_fn = "(stdin)";
  if (argc == 2) {
    input_fn = argv[1];
    input = fopen(input_fn, "re");
    if (!input) err(1, "%s", input_fn);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }
  
  startprocess.foregroundexstatus = 0;
  startprocess.foregroundpid = getpid();

  char *line = NULL;
  size_t n = 0;

int interactive = 0;

prompt:
  for (;;) {

    int backgroundStatus;
    int childPID;
    childPID = waitpid(0, &backgroundStatus, WUNTRACED);
    startprocess.backgroundpid = childPID;
    if(WIFEXITED(backgroundStatus)){
      fprintf(stderr, "Child process %d done. Exit status %d.\n", childPID, WIFEXITED(&backgroundStatus));
    } 

    /* TODO: prompt */ 
  if (input == stdin) {
      interactive = 1;
      
      sigaction(SIGTSTP, &ignore_action, &previousHandlerSIGTSTP);
      sigaction(SIGINT, &ignore_action, &previousHandlerSIGINT);
      
     
    fprintf(stderr, "%s", expand("${PS1}"));

    fflush(stderr);
    
    sigaction(SIGINT, &SIGINT_action, NULL);}

    ssize_t line_len = getline(&line, &n, input);
    if (feof(input))exit(0);
    
    if (line_len < 0) {
      err(0, "%s", input_fn);
      clearerr(stdin);
      goto prompt;}
    
    sigaction(SIGINT, &ignore_action, NULL);
    
    size_t nwords = wordsplit(line);

   
    /* Iterate and expand each word */
    for (size_t i = 0; i < nwords; ++i) {
      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;}

    /*
     *
     * CHECK for background operator.
     *
     */

     char *ampersand = "&";
     char *lastword = words[nwords-1];
     int backroundflag = 0;

    if(!strcmp(lastword, ampersand)){
      backroundflag = 1;}
    /*
     *
     *
     * Built-In Shell Commands CD & Exit
     *
     */

    /* EXIT */ 

    if (!strcmp(words[0], "exit")){
    // If exit has an argument:
      
      if(nwords==2){ 
        //Convert exit argument into a int.
        char *ptr;
        int ret;
        ret = strtol(words[1], &ptr, 10);
        startprocess.foregroundexstatus = ret;
        exit(ret);

      }if(nwords == 1){
        // EXIT with the status of the last foreground command
        exit(startprocess.foregroundexstatus); }

      else{
        //fprintf(stderr, "ERROR: Wrong input.\n");
        }
    }

    /* CHANGE DIRECTORY */
    else if (!strcmp(words[0], "cd")) {
        
      /*IF: input is provided*/
        if (nwords == 2){
          chdir(words[1]);}
        if (nwords == 2){
          chdir(words[1]);}
       
        /*IF: input is NOT provided */
        else if (nwords == 1){
          chdir(getenv("HOME"));
    }}


/*
 *
 * NON-Built-In_commands
 *
 */
    if (strcmp(words[0], "/")){
      
      //pid_t backroundpid = -4;
      pid_t spawnpid = -4;
      int childStatus;
    /* Fork process and run the execvp in the child process. */
      spawnpid = fork();
      
      switch(spawnpid){
       // CASE 1: Fork has failed
        case -1:{
          perror("fork() failed!");
          startprocess.foregroundexstatus = 1;
          exit(1);}
       
      // CASE 2: The child will execute the code in this branch.
        case 0:{
          /*
           *
           * CHECK FOR REDIRECTION
           *
           */
          if (interactive == 1){
          sigaction(SIGINT, &previousHandlerSIGINT, NULL);
          sigaction(SIGTSTP, &previousHandlerSIGTSTP, NULL);}
   
          char *filename1 = "SAVING SPACE";
          char *filename2 = "SAVING SPACE";
          char *filename3 = "SAVING SPACE";
          
          for(int i = 0; i < nwords ;i++){

            if(!strcmp(words[i], ">")){
              // OPEN file for writing to STDOUT
                filename1 = words[i+1];
                int targetFD = open(words[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
                if(targetFD == -1){
                  perror("target open()");
                  exit(1);}
                int result = dup2(targetFD, STDOUT_FILENO);
               

                if(result == -1){
                  perror("target dup2()");
                  exit(2);}
                fcntl(targetFD, F_SETFD, FD_CLOEXEC);
                }
            
            else if(!strcmp(words[i], ">>")){
              // OPEN file for appending on STDOUT  
                 filename2 = words[i+1];
                 int targetFD2 = open(words[i+1],O_WRONLY | O_CREAT | O_APPEND, 0777);
                 if (targetFD2 == -1){
                 perror("target open()");
                 exit(1);}

                 int result2 = dup2(targetFD2, STDOUT_FILENO);
               
                 if(result2 == -1){
                 perror("target dup2()");
                 exit(2);}
                 fcntl(targetFD2, F_SETFD, FD_CLOEXEC);
                  }

            else if (!strcmp(words[i], "<")){
              // OPEN file for reading on stdin
                fclose(stdin);
                filename3 = words[i+1];
                int sourceFD = open(words[i+1], O_RDONLY);
                if(sourceFD == -1){
                  perror("source open()");
                  exit(1);}
                
                int result = dup2(sourceFD, 0);
                if(result == -1){
                  perror("source dup2()");
                  exit(2);}
   
            }
                 }
            char *newarr[nwords+1];
            int a = 0;
            int len = nwords;


            for (int i = 0; i < nwords; i++){
              if(!strcmp(words[i], "&") || !strcmp(words[i], "<") || !strcmp(words[i], ">") || !strcmp(words[i], ">>") || !strcmp(words[i], filename1)|| !strcmp(words[i], filename2) || !strcmp(words[i], filename3)){
                a++;
                len -= 1;
                newarr[i] = words[a];
                continue;}
              newarr[i] = words[a];
              a++;}
             
              newarr[len] = 0;

            if(execvp(words[0], newarr) == -1){
              fprintf(stderr, "No such file or directory.\n");
              exit(1);}
               }     
      
        // CASE 3: The parent will wait for the child process to finish running.
        default:{

         if(backroundflag == 0){

          spawnpid = waitpid(spawnpid, &childStatus, WUNTRACED);

          if(WIFSTOPPED(childStatus)){
              kill(spawnpid, SIGCONT);
              fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnpid);
             startprocess.backgroundpid = spawnpid;
             backroundflag = 1;
             }
         
          if(WIFEXITED(childStatus)){
          //startprocess.backgroundpid = spawnpid;
            startprocess.foregroundexstatus = WEXITSTATUS(childStatus);
           }
         
          else if(WIFSIGNALED(childStatus)) {
           startprocess.foregroundexstatus = WTERMSIG(childStatus) + 128;
           }
               }

        /* else if (backroundflag == 1){

           int backroundpid = waitpid(spawnpid, &childStatus, WNOHANG);
           startprocess.backgroundpid = backroundpid;

         }*/
                }
         
      }
    }
  }
 return 0; 
}

char *words[MAX_WORDS] = {0};

/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;

  char const *c = line;
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) {
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      if (!tmp) err(1, "realloc");
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    for (;*c && isspace(*c); ++c);
  }
  return wind;
}


/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */
char
param_scan(char const *word, char const **start, char const **end)
{
  static char const *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = 0;
  *end = 0;
  for (char const *s = word; *s && !ret; ++s) {
    s = strchr(s, '$');
    if (!s) break;
    switch (s[1]) {
    case '$':
    case '!':
    case '?':
      ret = s[1];
      *start = s;
      *end = s + 2;
      break;
    case '{':;
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = s[1];
        *start = s;
        *end = e + 1;
      }
      break;
    }
  }
  prev = *end;
  return ret;
}
/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */
char *
build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0;

  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}/*
 * Returns a newly allocated string that the caller must free
 */
char *
expand(char const *word)
{
  char const *pos = word;
  char const *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);
  while (c) {
    // CHECK for $!
    if (c == '!'){
      if(startprocess.backgroundpid){
        pid_t pid = startprocess.backgroundpid;
        char *pidChar = malloc(100);
        sprintf(pidChar, "%d", pid);
        build_str(pidChar, NULL);}
      else{
        build_str("",NULL);}}
   
    // CHECK for $$
    else if (c == '$'){ 
      pid_t pid = getpid();
      char *pidChar = malloc(6);
      sprintf(pidChar, "%d", pid);
      build_str(pidChar, NULL);
      free(pidChar);
    }
    
    // CHECK for $?
    else if (c == '?'){ 
       int num = startprocess.foregroundexstatus;
       char *exstatus = malloc(50);
       sprintf(exstatus, "%d", num);
       build_str(exstatus, NULL);}
    
    // CHECK for parameter
    else if (c == '{') {
      
      char newenvar[200];
      char a;
     

      for(int i = 0;  ; i++){
        //if(strcmp(&start[2+i], "}")){
        //break;}
        a = start[2+i];
        if(a =='}'){
          newenvar[i] = 0;
          break;}
        else{
        newenvar[i] = a;
        }}
        if (getenv(newenvar) == NULL){
          build_str("", NULL);}
        else{
          build_str(getenv(newenvar), NULL);}
    }

    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }

  return build_str(start, NULL);
}
