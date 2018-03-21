#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFLEN 128
#define INSNUM 8
/*internal instructions*/
char *instr[INSNUM] = {"show","show","show","show","chdir","exit","wait","team"};
/*predefined variables*/
/*varvalue[0] stores the rootpid of xssh*/
/*varvalue[3] stores the childpid of the last process that was executed by xssh in the background*/
int varmax = 3;
char varname[BUFLEN][BUFLEN] = {"$\0", "?\0", "!\0",'\0'};
char varvalue[BUFLEN][BUFLEN] = {'\0', '\0', '\0'};
/*remember pid*/
int childnum = 0;
pid_t childpid = 0;
pid_t rootpid = 0;
/*current dir*/
char rootdir[BUFLEN] = "\0";

/*functions for parsing the commands*/
int deinstr(char buffer[BUFLEN]);
void substitute(char *buffer);
void parser(char* argument, char** argv); // ADD: helper function: parse the buffer into list of arguments and sent to program()

/*functions to be completed*/
int xsshexit(char buffer[BUFLEN]);
void show(char buffer[BUFLEN]);
void team(char buffer[BUFLEN]);
int program(char buffer[BUFLEN]);
void ctrlsig(int sig);
void changedir(char buffer[BUFLEN]);
void waitchild(char buffer[BUFLEN]);

/*for extra credit, implement the function below*/
int pipeprog(char buffer[BUFLEN]);

/*main function*/
int main()
{
	/*set the variable $$*/
	rootpid = getpid();
	childpid = rootpid;
	sprintf(varvalue[0], "%d\0", rootpid);
	/*capture the ctrl+C*/
	if(signal(SIGINT, ctrlsig) == SIG_ERR)
	{
		printf("-xssh: Error on signal ctrlsig\n");
		exit(0);
	}
	/*run the xssh, read the input instrcution*/
	int xsshprint = 0;
	if(isatty(fileno(stdin))) xsshprint = 1;
	if(xsshprint) printf("xssh>> ");  /* prompt */
	char buffer[BUFLEN];
	while(fgets(buffer, BUFLEN, stdin) > 0)
	{
		/*substitute the variables*/
		substitute(buffer);
		/*delete the comment*/
		char *p = strchr(buffer, '#');
		if(p != NULL)
		{
			*p = '\n';
			*(p+1) = '\0';
		}
		/*decode the instructions*/
		int ins = deinstr(buffer);
		/*run according to the decoding*/
		if(ins == 1)
			show(buffer);
		else if(ins == 2) show(buffer); //Not used for now
		else if(ins == 3) show(buffer); //Not used for now
		else if(ins == 4) show(buffer); //Not used for now
		else if(ins == 5)
			changedir(buffer);
		else if(ins == 6)
			return xsshexit(buffer);
		/*	{				// debug code for exit()
			int value = 0;
			value = xsshexit(buffer);
			printf("xsshexit returned value: %d \n", value);
		}	*/
		else if(ins == 7)
			waitchild(buffer);
		else if(ins == 8)
			team(buffer);
		else if(ins == 9)
			continue;
		else  /* code needed for pipeprog() */
		{
			char *ptr = strchr(buffer, '|');
			if(ptr != NULL)
			{
				int err = pipeprog(buffer);
				if(err != 0)break;
			}
			else
			{
				int err = program(buffer);
				if(err != 0)break;
			}
		}
		if(xsshprint) printf("xssh>> ");  /* prompt in while loop */
		memset(buffer, 0, BUFLEN); /* clean and reset buffer = 000000... */
	}
	return -1;
}

/*exit I*/
int xsshexit(char buffer[BUFLEN])
{
	int i; /* pointer for buffer[5] - buffer[strlen(buffer)] */
	char argI[strlen(buffer)-5]; /* create an argument 'argI' with length of 5-less than buffer string */
	int flag = 0; /* if argI is valid, set flag to 1 */
	int j = 0; /* j is pointer of argI */
	int start = 5;
	
	while(buffer[start]==' ')start++;
	for(i = start; (buffer[i]!='\n')&&(buffer[i]!=' '&&(i<BUFLEN))&&(buffer[i]!='\0'); i++)  /* loop pointer i FROM first non-space character TO end of first argument = argI */
	{
		argI[j] = buffer[i];
		j++;
	}
	// printf("argI is %s \n", argI);
	// printf("argI int is %d \n", atoi(argI));
	
	if (strlen(buffer) == 5)return 0; // if the command is only "exit", return 0
	
	for(j = 0; j < strlen(argI); j++)
	{
		if ( (argI[j] < '0') || (argI[j] > '9') || (argI[j] == ' ') ) // if argI contains space or non-number, set flag = 0 and end for loop
		{
			flag = 0;
			break;	
		} else {
			flag = 1;
		}
	}

	if (flag == 1) {
		return atoi(argI);
	} else {
		return -1;
	}
	/* FIXED */
}

/*show W*/
void show(char buffer[BUFLEN])
{
	int len = strlen(buffer);
	int i;
	for (i = 5; i < len; i++) {
		printf("%c", buffer[i]);
	}
	//FIXED
}

/*team T*/
void team(char buffer[BUFLEN])
{
	printf("Team members: %s; %s; %s\n", "Yinxia Li", "Liansai Dong", "Yuchen Peng");
	//FIXED
}

/*chdir D*/
void changedir(char buffer[BUFLEN])
{
        int i, j;
        int flag = 0;
	int start = 6;
	while(buffer[start]==' ')start++;

	/*store the directory in rootdir*/
	char temp[BUFLEN];
	strcpy(temp, rootdir);
        for(i = start; (i < strlen(buffer)&&(buffer[i]!='\n')&&(buffer[i]!='#')); i++)
        {
                rootdir[i-start] = buffer[i];
        }
        rootdir[i-start] = '\0';

	struct stat sb;
	if (stat(rootdir, &sb) == 0 && S_ISDIR(sb.st_mode)) { //FIXED: changes the current working dir. of xssh to the directory specified in rootdir and print "-xssh: change to dir 'rootdir'\n"
		printf("-xssh: change to dir %s\n", rootdir);
	} else { 	//FIXED: if rootdir not exist, print error message "-xssh: chdir: Directory 'D' does not exist"
		printf("-xssh: chdir: Directory %s does not exist\n", rootdir);
		strcpy(rootdir, temp);
	}
}

/*ctrl+C handler*/
void ctrlsig(int sig)
{
	if (childpid != rootpid) //FIXED: check if the foreground process is xssh itself
	{
		kill(childpid, SIGKILL); //FIXED: if not xssh itself, kill the foreground process and print "-xssh: Exit pid &childpid"
		printf("-xssh: Exit pid: %d \n", childpid);
		childpid = rootpid;
		fflush(stdout); // ??
	}
}

/*wait instruction*/
void waitchild(char buffer[BUFLEN])
{
	int i;
	int start = 5;
	int w_status;
	pid_t wpid;

	/*store the childpid in pid*/
	char number[BUFLEN] = {'\0'}; 
	while(buffer[start]==' ')start++;
	for(i = start; (i < strlen(buffer))&&(buffer[i]!='\n')&&(buffer[i]!='#'); i++)
	{
		number[i-start] = buffer[i];
	}

	char *endptr;
	int pid = strtol(number, &endptr, 10);

	/*simple check to see if the input is valid or not*/
	if((*number != '\0')&&(*endptr == '\0'))
	{
		if (pid != -1 && pid > 0) { 	//FIXED: if pid is not -1, try to wait the background process pid
			wpid = waitpid(pid, &w_status, 0);
			if(WIFEXITED(w_status) && wpid != -1) //FIXED: if successful, print "-xssh: Have finished waiting process $pid"
			{
 				printf("-xssh: Have finished waiting process: %d. \n", wpid); 
			} else { //FIXED: if not successful, print "-xssh: Unsuccessfully wait the background process $pid"
				printf("-xssh: Unsuccessfully wait the background process: %d.\n", pid);
			}
		} else if (pid == -1){ 	//FIXME: if pid is -1, print "-xssh: wait %childnum background processes", and wait all the background processes
			printf("-xssh: wait %d background processes: \n\n", childnum);
			wpid = waitpid(-1, &w_status, 0);
		}
		//hint: remember to set the childnum correctly after waiting (??)
		// Q : when the waitpid() returns, is the child process continued or terminated?
	}
	else printf("-xssh: wait: Invalid pid\n");
}

/*execute the external command*/
int program(char *buffer)
{
	/*if backflag == 0, xssh need to wait for the external command to complete*/
	/*if backflag == 1, xssh need to execute the external command in the background*/
	int backflag = 0;
	char *ptr = strchr(buffer, '&');
	if(ptr != NULL) backflag = 1;

	// parse the command from the buffer
	char **argv;
	argv = (char**)malloc(sizeof(char*)*BUFLEN);	
	parser(buffer, argv);
	printf("parsed argv (command): %s \n", *argv);	// TODO remove before submission

	pid_t pid;
	pid = fork(); // FIXED: create a new process
	if (backflag)
		childnum++; // support command "wait -1"
	if (pid < 0) // FIXED: check if the process creation is successful
	{
		printf("xssh: failed to fork/create process.");
		childnum--;
	}
	else if (pid == 0) { // FIXED: child process, execute the external command
		if (execvp(*argv, argv) == -1) // FIXED: check if the external command is executed successfully
		{
			childnum--; // ?
			printf("xssh: Unable to execute the instruction: %s. \n", *argv);
			return -1;
		}
		childnum--;
	//hint: the external command is stored in buffer, but before execute it you may need to do some basic validation check or minor changes, depending on how you execute
	} else { // parent process, pid > 0
		if (backflag == 1) // FIXED: act differently based on backflag
		{ // background
			printf("process running in background. \n");
			childpid = pid;
			// while (waitpid(-1, NULL, 0) > 0)
			sprintf(varvalue[2], "%d\0", pid); // support command "show $!"
		}
		else
		{ // foreground
			printf("process running in foreground. \n");
			waitpid(pid, NULL, 0);
		}
	}
	/* TODO for extra credit, implement stdin/stdout redirection in here*/
	return 0;
}

/* TODO for extra credit, implement the function below*/
/*execute the pipe programs*/
int pipeprog(char buffer[BUFLEN])
{
	printf("-xssh: For extra credit: currently not supported.\n");
	return 0;
}

/*substitute the variable with its value*/
void substitute(char *buffer)
{
	char newbuf[BUFLEN] = {'\0'}; /* '\0' is the null-terminator */
	int i;
	int pos = 0; /* pos is pointer of newbuf */
	for(i = 0; i < strlen(buffer);i++)
	{
		if(buffer[i]=='#') /* replace '#' with '\n', then skip = NOT parsing the rest of the string */
		{
			newbuf[pos]='\n';
			pos++;
			break;
		}
		else if(buffer[i]=='$')
		{
			if((buffer[i+1]!='#')&&(buffer[i+1]!=' ')&&(buffer[i+1]!='\n'))
			{
				i++; 
				int count = 0; /* count is index of tmp */
				char tmp[BUFLEN];
				for(; (buffer[i]!='#')&&(buffer[i]!='\n')&&(buffer[i]!=' '); i++)  /* loop pointer i from i+1 to end of $(VARNAME)\0 */
				{
					tmp[count] = buffer[i];
					count++;
				} /* copy the $(VARNAME) into tmp, move pointer i to end of $VARNAME, move pointer i to end of $(VARNAME) */
				tmp[count] = '\0'; 
				int flag = 0;
        			int j;
				for(j = 0; j < varmax; j++) /* looking for parsed tmp = 'VARNAME' in global var. list varname */
        			{
                			if(strcmp(tmp,varname[j]) == 0)
					{
						flag = 1;
						break;
                			}
        			}
        			if(flag == 0) /* if not found in var. list, prompt */
        			{
					printf("-xssh: Does not exist variable $%s.\n", tmp);
        			}
        			else /* if found in var. list, strcat() append varvalue[j] = the found var. to &newbuf[pos], move pointer pos to end of newbuf */
				{
					strcat(&newbuf[pos], varvalue[j]);
					pos = strlen(newbuf);
        			}
				i--; /* shift pointer back 1 position, pairing with the 'i++' at the beginning of the loop */
			}
			else /* if buffer[i+1] is '#', space or newline, copy paste the '$' to newbuf */
			{
				newbuf[pos] = buffer[i];
				pos++;
			}
		}
		else /* copy paste the current character to newbuf */
		{
			newbuf[pos] = buffer[i];
			pos++;
		}
	}
	if(newbuf[pos-1]!='\n') /* ??? */
	{
		newbuf[pos]='\n';
		pos++;
	}
	newbuf[pos] = '\0';
	strcpy(buffer, newbuf); /* copy the value in newbuf back to buffer */
	printf("Decoded: %s", buffer); /* TODO: COMMENT THIS LINE BEFORE SUBMISSION */
}

/*decode the instruction*/
int deinstr(char buffer[BUFLEN])
{
	int i;
	int flag = 0;
	for(i = 0; i < INSNUM; i++) /* INSNUM = 8; char *instr[INSNUM] = {"show","show","show","show","chdir","exit","wait","team"}; */
	{
		flag = 0;
		int j;
		int stdlen = strlen(instr[i]);
		int len = strlen(buffer);
		int count = 0;
		j = 0;
		while(buffer[count]==' ')count++; /* skip all space(s) before command */
		if((buffer[count]=='\n')||(buffer[count]=='#')) /* if command is newline, or command is comment, then set flag = 0, i = 8 */
		{
			flag = 0;
			i = INSNUM;
			break;
		}
		for(j = count; (j < len)&&(j-count < stdlen); j++)
		{
			if(instr[i][j] != buffer[j])
			{
				flag = 1; /* deinstr() will return ins = 0, main() will execute nothing */
				break;
			}
		}
		if((flag == 0) && (j == stdlen) && (j <= len) && (buffer[j] == ' ')) /* jump out of for loop with flag = 0 if next char at buffer is space */
		{
			break;
		}
		else if((flag == 0) && (j == stdlen) && (j <= len) && (i == 5)) /* jump out of for loop with flag = 0 if command is exit command AND there are additional char's following 'exit' */
		{
			break;
		}
		else if((flag == 0) && (j == stdlen) && (j <= len) && (i == 7)) /* jump out of for loop with flag = 0 if command is team command AND there are additional char's following 'team' */
		{
			break;
		}
		else
		{
			flag = 1; /* deinstr() will return ins = 0, main() will execute nothing */
		}
	} /* end for loop */

	if(flag == 1)
	{
		i = 0; /* deinstr() will return ins = 0, main() will execute nothing */
	}
	else
	{
		i++;
	}
	return i;
}

void parser(char* argument, char** argv)
{	
	int j = 0;
	char* token;
	token = strtok(argument, " ,.\n"); 	// tokenize the arguments
	while (token != NULL){
		argv[j] = token;
		token = strtok(NULL, " ,&\n");	// '&' will be removed
		printf("argv[j] is %s. \n", argv[j]); // TODO remove before submission
		j++;
	}
argv[j] = NULL;
}
