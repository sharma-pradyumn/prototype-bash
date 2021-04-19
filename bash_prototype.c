
#include  <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <signal.h>
#include <fcntl.h>  
#include <errno.h>
#include <limits.h>

#define BUFFER_SIZE 4096  // Buffer length upto which input can be taken.      

#ifndef READ
#define READ 0
#endif

#ifndef WRITE
#define WRITE 1
#endif


// Linked list for history 
struct node 
{
  int index;
  char cmd[BUFFER_SIZE];
  struct node *next;
};

struct node *start = NULL;


// Various helper functions for the linked lists
void insert_at_begin(int x,char* new_cmd) {
  struct node *t;

  t = (struct node*)malloc(sizeof(struct node));
  t->index = x;
  strcpy(t->cmd,new_cmd);

  // count++;

  if (start == NULL) {
    start = t;
    start->next = NULL;
    return;
  }

  t->next = start;
  start = t;
}


void insert_at_end(int x,char* new_cmd) {
  struct node *t, *temp;

  t = (struct node*)malloc(sizeof(struct node));
  t->index = x;
  strcpy(t->cmd,new_cmd);

  if (start == NULL) {
    start = t;
    start->next = NULL;
    return;
  }

  temp = start;

  while (temp->next != NULL)
    temp = temp->next;

  temp->next = t;
  t->next   = NULL;
}

char* retrieve_by_index(int new_index) 
{
  struct node *t;

  t = start;

  if (t == NULL) {
    printf("Linked list is empty.\n");
    return "NONE";
  }

  while (t!=NULL && t->index!=new_index) 
  {
    t = t->next;
  }
  if(t==NULL)
          return "NONE";

  return t->cmd;   
}



//Linked list for status

/* 

Status -> 1 for active process, 0 for completed

*/


struct pid_node 
{
  pid_t pid_n;
  int process_status;
  char cmd[BUFFER_SIZE];
  struct pid_node *next;
};

struct pid_node *pid_start = NULL;



//called while spawing a new process by insert_at_brgin_pid(new_pid,1,argss[0])
void insert_at_begin_pid(pid_t new_pid,int x,char* new_cmd) 
{

  struct pid_node *t;

  t = (struct pid_node*)malloc(sizeof(struct pid_node));
  
  t-> pid_n=new_pid;
  t->process_status = x;
  strcpy(t->cmd,new_cmd);

  if (pid_start == NULL) {
    pid_start = t;
    pid_start->next = NULL;
    return;
  }

  t->next = pid_start;
  pid_start = t;
}

//called while spawing a new process by insert_at_end_pid(new_pid,1,argss[0])
void insert_at_end_pid(pid_t new_pid,int x,char* new_cmd) {
  struct pid_node *t, *temp;

  t = (struct pid_node*)malloc(sizeof(struct pid_node));
  
  t-> pid_n=new_pid;
  t->process_status = x;
  strcpy(t->cmd,new_cmd);

  if (pid_start == NULL) {
    pid_start = t;
    pid_start->next = NULL;
    return;
  }

  temp = pid_start;

  while (temp->next != NULL)
    temp = temp->next;

  temp->next = t;
  t->next   = NULL;
}

//Fetches all processes executed in the shell
void fetch_all_process() 
{
  struct pid_node *t;
  t = pid_start;

  if (t == NULL) {
    printf("No process spawned.\n");
  }

  while (t!=NULL) 
  {
    printf("command name : %s process id :%d\n",t->cmd,t->pid_n);
    t = t->next;
  }
    
}

//Fetches all active processes in the shell
void fetch_active_process() 
{
  struct pid_node *t;
  t = pid_start;

  if (t == NULL) {
    printf("No process spawned.\n");
  }

  while (t!=NULL) 
  {
    if (t->process_status==1)
    {printf("command name : %s process id :%d\n",t->cmd,t->pid_n);}
    t = t->next;
  }
    
}

//set a status as completed by calling set_status(pid,0)
void set_status(pid_t pid_to_change,int status_new)
{

  struct pid_node *t;

  t = pid_start;

  if (t == NULL) {
    printf("Cannot set status, no process spawned.\n");
  }

  while (t!=NULL && t->pid_n!=pid_to_change) 
  {
    t = t->next;
  }
  if(t==NULL)
      printf("No such process\n");

  t->process_status=status_new;   
  
}

// It is a function similar to strtok, it replaces all spaces,tabs and new line with \0 and for all other char, 
//it adds a pointer to argss. However, it also returns a counter, which is the number of tokens generated
int  parse(char* line, char** argss)
{    
     int counter=0;
     while (*line !=NULL) 
     {        
          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     
          
          if (*line !=NULL)
          {*argss++ = line;          
          counter++;}
          
          while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n') 
          {    
              // if (*line=='|' || *line=='>' || *line=='<')
              //  {*argss++ = line;          
              //   counter++;
              //  }

              line++;
              
          }             
     }

     *argss = '\0';                 

     return counter;
}

void execute_background(char **argss)
{
     pid_t  child_pid;
     int    status;
     
     child_pid=fork();

     if (child_pid < 0) {     /* fork a child process           */
          printf("*** ERROR: forking child process failed\n");
          _exit(1);
     }


     else if (child_pid == 0) 
     {          /* for the child process:         */
          if (execvp(*argss,argss) < 0) 
          {     /* execute the command  */
               printf("*** ERROR: EXEC failed\n");
               _exit(1);
          }
          
          _exit(0);
     }

     //parent
     else {                                  /* for the parent:      */
          

          insert_at_end_pid(child_pid,1,argss[0]);
          printf("%d child spawned in background\n",child_pid);
     }
}



void  execute_foreground(char **argss)
{
     pid_t  child_pid;
     int    status;
     char line_check[BUFFER_SIZE];
     int in_counter=-1;
     int out_counter=-1;
     int counter=0;
     int pipes=0;
     int arg_len;
     int pipe_location[64];
     char temp_str[BUFFER_SIZE];
     char temp_str2[BUFFER_SIZE];

     int fd0,fd1,i,in=0,out=0;
     char input[NAME_MAX],output[NAME_MAX];
     int xx=0;
     int j=0;


     int l_pipe[2], r_pipe[2];


     pipe_location[0]=0;


     char *comd_args[64];

     for (i=0;argss[i]!=NULL;i++)
     {
          if (argss[i][0]=='|' && strcmp(argss[i], "|") != 0)
          {
             comd_args[j]="|";
             j++;
             strcpy(temp_str,argss[i]);
             strcpy(temp_str2,temp_str+1);
             comd_args[j]=temp_str2;
             printf("%s\n",comd_args[j]);
             j++;
          }

          else if (argss[i][0]=='<' && strcmp(argss[i], "<") != 0) 
          {
             comd_args[j]="<";
             j++;
             strcpy(temp_str,argss[i]);
             strcpy(temp_str2,temp_str+1);
             comd_args[j]=temp_str2;
             printf("%s\n",comd_args[j]);
             j++;
          }

          else if (argss[i][0]=='>' && strcmp(argss[i], ">") != 0) 
          {
             comd_args[j]=">";
             j++;
             strcpy(temp_str,argss[i]);
             strcpy(temp_str2,temp_str+1);
             comd_args[j]=temp_str2;
             printf("%s\n",comd_args[j]);
             j++;
          }

          else if (argss[i][arg_len-1]=='<' && strcmp(argss[i], "<") != 0)
          {
              argss[i][arg_len-1]=NULL;
              argss[i][arg_len-1]=NULL;
              comd_args[j]=argss[i];
              comd_args[j+1]="<";
              j=j+2;
          }

          else if (argss[i][arg_len-1]=='>' && strcmp(argss[i], ">") != 0)
          {

              printf("> at end\n");
              argss[i][arg_len-1]=NULL;
              comd_args[j]=argss[i];
              comd_args[j+1]=">";
              j=j+2;
              
          }

          else if (argss[i][arg_len-1]=='|' && strcmp(argss[i], "|") != 0)
          {
              argss[i][arg_len-1]=NULL;
              comd_args[j]=argss[i];
              comd_args[j+1]="|";
              j=j+2;
          }

          else
          {
            comd_args[j]=argss[i];
            j++;
          }
          

     }

     counter=0;

     for (int j = 0;comd_args[j]!=NULL; ++j)
     {
       counter++;
     }

     for(i=0;comd_args[i]!=NULL;i++)
    
    {
                  if (strcmp(comd_args[i], "|") == 0) 
                  {
                    
                    if (i < counter-1)
                    { 
                          comd_args[i]=NULL;
                          ++pipes;
                          i++;
                          pipe_location[pipes]=i;
                    }
                    else
                    {
                        printf("Last token Cannot be |\n");
                        return;
                    }

                  }
                    
                  if(strcmp(comd_args[i],"<")==0)
                  {        
                      
                      if (i < counter-1)
                      {
                        comd_args[i]=NULL;
                        in_counter=i;
                        strcpy(input,comd_args[i+1]);
                        in=2;
                        printf("Taking input from %s\n",input);
                        i++;
                      }
                      else
                      {
                        printf("Last token Cannot be < , please give filename\n");
                        return; 
                      }
                                 
                  }               

                  if(strcmp(comd_args[i],">")==0)
                  {      
                       if (i < counter-1)
                      {
                        comd_args[i]=NULL;
                        out_counter=i;
                        strcpy(output,comd_args[i+1]);
                        out=2;
                        printf("Giving output to %s\n",output);
                        i++; 
                      }
                      else
                      {
                        printf("Last token Cannot be > , please give filename\n");
                        return; 
                      }

                  }


      
    }

    for (int index = 0; index <= pipes; ++index) 
    {
                                  if (pipes > 0 && index != pipes) 
                                  { /* if user has entered multiple commands with '|' */
                                      pipe(r_pipe); /* no pipe(l_pipe); r_pipe becomes next child's l_pipe */
                                  }


                                  child_pid=fork();
                                  
                                   if (child_pid<0)
                                   {     /* fork a child process           */
                                        perror("fork failed"); /* fork() error */
                                        return;
                                    }


                                  else if (child_pid==0)//CHILD
                                  {
                                                          if ( (index==0) && in)
                                                          {          
                                                                int fd0;
                                                                if ((fd0 = open(input, O_RDONLY)) < 0) 
                                                                {
                                                                    perror("Couldn't open input file");
                                                                    exit(0);
                                                                }           
                                                                // dup2() copies content of fdo in input of preceeding file
                                                                dup2(fd0,STDIN_FILENO); // STDIN_FILENO here can be replaced by 0 

                                                                close(fd0); // necessary
                                                          }

                                                          if ((index == pipes) && out)
                                                          {
                                                            int fd1;
                                                            if ((fd1 = creat(output, 0700) ) < 0) 
                                                            {
                                                                perror("Couldn't open the output file");
                                                                exit(0);
                                                            }           

                                                            dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
                                                            close(fd1);

                                                          }

                                                          if (pipes > 0) 
                                                          {              
                                                                    if (index == 0)
                                                                    { /* first child process */
                                                                        //close(WRITE);
                                                                        dup2(r_pipe[WRITE],WRITE);
                                                                        close(r_pipe[WRITE]);
                                                                        close(r_pipe[READ]);
                                                                    }
                                                                    else if (index < pipes) 
                                                                    { /* in-between child process */
                                                                        //close(READ);
                                                                        dup2(l_pipe[READ],READ);
                                                                        
                                                                        //close(WRITE);
                                                                        dup2(r_pipe[WRITE],WRITE);

                                                                        close(l_pipe[READ]);
                                                                        close(l_pipe[WRITE]);
                                                                        close(r_pipe[READ]);
                                                                        close(r_pipe[WRITE]);
                                                                    }
                                                                    else 
                                                                    { /* final child process */
                                                                        //close(READ);
                                                                        dup2(l_pipe[READ],READ);
                                                                        close(l_pipe[READ]);
                                                                        close(l_pipe[WRITE]);
                                                                    }
                                                          }

                                                           xx=pipe_location[index];
                                                          //  /* execute command */ 
                                                           //printf("Executing %s \n",argss[xx]);
                                                           if (   (execvp((comd_args[xx]), comd_args+xx) < 0)    ) 
                                                           {perror("execution of command failed\n");
                                                            exit(1);
                                                           }
                                                           /* if execvp() fails */
                                                           // perror("execution of command failed\n");
                                  }

                                  else{ /* parent process manages the pipes for child process(es) */
                                        if (index > 0) 
                                        {
                                            close(l_pipe[READ]);
                                            close(l_pipe[WRITE]);
                                        }
                                        l_pipe[READ] = r_pipe[READ];
                                        l_pipe[WRITE] = r_pipe[WRITE];

                                        insert_at_end_pid(child_pid,1,argss[pipe_location[index]]);
                                        printf("spawned process %d to execute %s\n",child_pid,argss[pipe_location[index]]);
                                        waitpid(child_pid, &status, 0);
                                      }
    }

return;
     
}
  

// Special function to explicitly check that all child processes have terminated
int wait_and_poll()
{
    int *cstatus;
    int status;
    

    while ((status = waitpid(-1,cstatus,WNOHANG))>0){
        fprintf(stdout,"%ld Terminated.\n",(long) status);
    }
return 0;
}


// Handles the SIGCHILD signal, it searches through the linked list and sets status of the given process to completed
void sigchld_handler (int sig) {
    if (sig==SIGCHLD)
    {
      int status;
      pid_t c;
      while ((c = waitpid(-1, &status, WNOHANG)) > 0) 
      {
          set_status(c,0);
          printf("Following child has exited :%d \n",c);
      }
    }
}

//Handles the interupt signal,to interupt all child processes,(like ping etc., Crtl+C will kill all child processes)
void handle_sigint(int sig) 
{ 
    printf("Caught signal %d\n", sig);
    struct pid_node *t;
    t = pid_start;
    printf("Starting kill\n");
    if (t==NULL)
    {
      return;
    }
    printf("Continuing\n");

    if (t->next!=NULL)
    {    t=t->next;}
    else
      {return;}
    printf("Moving on\n");
    while(t!=NULL)
    {
      printf("Killing the pid :%d which was executing %s\n",t->pid_n,t->cmd);
      kill(t->pid_n,SIGINT); 
      t=t->next;
    }
} 


//Main

int  main(int argc, char const *argv[])
{
     char  line[BUFFER_SIZE];             /* the input line                 */
     char  *argss[64];              /* the command line argument      */

     int cmd_index=1;     //The index number that will be assigned to the next execued instruction
     
     int check;           //Used to store the index number called in !HIST
     char* temp2;
     char* temp3;              // Temp3,4,5
     char temp4[BUFFER_SIZE]; // These are temporary placeholders, used for retreving commands from history,and before sending the string for parsing
                              //It is necessary to preserve a copy as parsing replaces all spaces,tabs,\n etc with NULL  
     char temp5[BUFFER_SIZE];
     int count;                  // It holds the number of tokens generated by parsing the current command

     char cwd[PATH_MAX];        //String variables to hold the current directory, home directory and the username
     char home[PATH_MAX];
     char username[NAME_MAX];

     int status;
     size_t last_len;            // holds the length of the last token generated, helps in checking for the presence of & in the end of the string
     int already_inserted=0;     //Checks if the current command has been inserted into History, Used in the case of !HIST to prevent multiple insertion
     
     //argss is an array of char pointers to NULL terminated strings, 
     // *++argss first increases the "pointer" to point to the next entry in the array argss 
     // (which the first time in the loop will be the first command line argument) 
     // and dereferences that pointer.

     //For example, for ls -al, argss is an array
     
     signal(SIGCHLD,sigchld_handler); //Signal handlers for SIGINT and SIGCHLD
     signal(SIGINT, handle_sigint);
     

     printf("Entering into Interactive Mode\n");



     insert_at_end_pid(getpid(),1,argv[0]);     //Insert the main process in the list of processes  



    //WRITE curr_dir into home
    if (getcwd(home, sizeof(home)) == NULL)     // Set current directory as the home directory
    {perror("getcwd() error");
     return 1;} 
    
   if (getlogin_r(username,sizeof(username)))  // Obtain the username 
   {perror("getlogin_r error");
    return 1;} 
    



     while (1) 
     {                   
          for (int jj=0;jj<64;jj++)
          {
            argss[jj]=NULL;                   //cleaning argss
          }

          already_inserted=0;
          if (getcwd(cwd, sizeof(cwd))==NULL)             /* Printing the current directory and the username */
          {
                perror("getcwd() error");
                return 1;
          }
          printf("<%s:%s>",username,cwd);
          if (fgets(line,BUFFER_SIZE,stdin)==NULL)
          { printf("Exiting the program\n");
            exit(0);
          }
          if ( feof(stdin))  //If EOF or NULL, exit the program
          { 
            printf("Exiting the program\n");
            exit(0); }

          printf("\n");
          size_t length = strlen(line);
          if (line[length - 1] == '\n') //replace the last escape character with null
          {  line[length - 1] = '\0';}
          
          strcpy(temp4,line);       //need to preserve a copy as parsing replaces all spaces,tabs,\n etc with NULL
          count=parse(line, argss);       /*   parse the line               */

                                        //temp4 will hold the string that the program will execute. In case of !HIST, the input string will be
                                        //replaced by the relevent command

          /* Executing the instruction */
          
          
          // If blank inut,continue
          if (count==0)
          {continue;}
          if ((count==1) && (strcmp(argss[0],"&") ==0))
          {printf("Please give a command to run in the background\n");   
          continue;}

          last_len=strlen(argss[count-1]);

// CHECK FOR !HIST
          if ((strcmp(argss[0],"!HIST") ==0)|| (strncmp(temp4,"!HIST",5)==0) )
          {
                temp3=temp4+5;
                check = atoi(temp3); // Obtain the index number of the command to execute
                if (check==0)
                  {printf("Please enter a number\n");}
                
                else if (check>=cmd_index)
                {printf("No command at given index\n");}

                else
                {
                  printf("executing command %s\n",retrieve_by_index(check)); //Retrieve the relevent command
                  strcpy(temp5,retrieve_by_index(check));
                  strcpy(temp4,temp5); // Copy it to temp4,so it can be executed by the shell

                  insert_at_begin(cmd_index,temp5); // add to history
                  cmd_index++;
                  already_inserted=1;
                  count=parse(temp5, argss);
                
                }
          }


          if ((strcmp(argss[count-1], "&") == 0) || argss[count-1][last_len-1]=='&')
          {
                        printf("Running in background\n");
                         //copy the entire string to temp3 and saving as it is(with spaces instead of NULL)
                         temp3=temp4;
                          //Integrating history command
                          if (already_inserted==0)
                          {    insert_at_begin(cmd_index,temp4); //to add to history
                               cmd_index++;
                               already_inserted=1;
                          }

                         if (strcmp(argss[count-1], "&") == 0) //Before sending the argss to execution, need to remove &
                         {
                          argss[count-1]=NULL;
                         } 
                         else if (argss[count-1][last_len-1]=='&')
                        { 
                          argss[count-1][last_len-1]='\0';
                        }
                         printf("%s\n", temp4);
                         execute_background(argss);       //Sending the command so it can be executed. 1 as second argument imply run as background
                              
          }

          else{

                              if (strcmp(argss[0],"pid") ==0) 
                              {
                                  //PID
                                            if (count==1)
                                            {
                                              printf("command name: %s process id: %d \n",argv[0],getpid());
                                            }
                                            else
                                            {
                                                if (strcmp(argss[1],"all") ==0)
                                                {fetch_all_process();}
                                                else if (strcmp(argss[1],"current") ==0)
                                                {fetch_active_process();}
                                                else
                                                  {printf("Please enter valid command\n");}

                                            }

                              }

                              else if (strcmp(argss[0],"cd") ==0) 
                              {
                                  //CD

                                              if (count==1)
                                              {
                                                printf("Please enter directory name\n");
                                                continue;
                                              }
                                              else if (strcmp(argss[1],"~") ==0)
                                              {

                                                  int ret = chdir(home); 
                                                  if (ret){ // same as ret!=0, means an error occurred and errno is set 
                                                        fprintf(stderr, "error: %s\n", strerror(errno)); 
                                                        }

                                              }
                                              else
                                              {   //change directory
                                                  int ret = chdir(temp4+3); 
                                                  if (ret){ // same as ret!=0, means an error occurred and errno is set 
                                                        fprintf(stderr, "error: %s\n", strerror(errno)); 
                                                        }
                                              }
                              }

                              else if ((strcmp(argss[0],"HIST") ==0)|| (strncmp(temp4,"HIST",4)==0) )
                              {
                                    temp3=temp4+4;
                                    check = atoi(temp3); //Obtain the index number
                                    if (check==0)
                                      {printf("Please enter a number\n");}
                                    else
                                    {printf("Listing the upto %d commands in history \n",check);
                                        for (int x = 1 ; x <cmd_index;x++)
                                        {     if (x>check)
                                                    break;
                                              printf("%d %s \n",cmd_index-x,retrieve_by_index(cmd_index-x));  //displaying the previously executed shell cmds
                                        }

                                    }

                              }
                          
                              //STOP
                              else if (strcmp(argss[0], "STOP") == 0)  /* is it an "exit"?     */
                                   {
                                    wait_and_poll(); //Make sure all child process terminate
                                    exit(0);
                                   }//exit if it is 

                              //EXECUTE NORMAL COMMAND
                              else
                              {       
                                      if (already_inserted==0)
                                      {  insert_at_begin(cmd_index,temp4); //to add to history
                                        cmd_index++;
                                        already_inserted=1;
                                      }
                                      parse(temp4, argss); 
                                      execute_foreground(argss);
                              }
                
                }

     }



}
