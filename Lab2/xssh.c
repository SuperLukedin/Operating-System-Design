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
	int value = 0; /* TODO added declaration */
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
		{
			value = xsshexit(buffer);
			printf("xsshexit returned value: %d \n", value);
		}
			/* TODO use this before assignment:  return xsshexit(buffer); */
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
				return 0; //TO DO 
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
	char argI[strlen(buffer)-5]; /* create an argument argI with length of 5-less than buffer string */
	int flag = 0; /* if argI is valid, set flag to 1 */
	int j = 0; /* j is pointer of argI */
	int space = 0;

	for(i = 5; (buffer[i]!='\n')&&(buffer[i]!=' '&&(i<BUFLEN))&&(buffer[i]!='\0'); i++)  /* loop pointer i from i+1 to end of $(VARNAME)\0 */
	{
		argI[j] = buffer[i];
		j++;
	}
	printf("argI is %s \n", argI);
	// printf("argI int is %s \n", atoi(argI));
	
	if (strlen(buffer) == 5){
		return 0;
	}
	
	for(j = 0; j < strlen(argI); j++)
	{
		/*	printf("evaluating: <%c> \n", argI[j]); // handling spaces
		if (argI[j] == ' ')
		{
			space++;
			printf("space detected");
			continue;
		} */
		if ( (argI[j] < '0') || (argI[j] > '9') )
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
	printf("\n");
	//FIXED
}

/*team T*/
void team(char buffer[BUFLEN])
{
	printf("Team members: %s; %s; %s\n", "Yinxia Li", "Liansai Dong","Yuchen Peng");
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
	if (stat(rootdir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
		printf("-xssh: change to dir %s\n", rootdir);
	} else {
		printf("-xssh: chdir: Directory %s does not exist\n", rootdir);
		strcpy(rootdir, temp);
	}

	//FIXME: changes the current working directory of xssh to the directory specified in rootdir and print "-xssh: change to dir 'rootdir'\n"
	//FIXME: if rootdir does not exist, print an error message "-xssh: chdir: Directory 'D' does not exist"
}

/*ctrl+C handler*/
void ctrlsig(int sig)
{
	//FIXME: first check if the foreground process (pid stored in childpid) is xssh itself (pid stored in rootpid)
	//FIXME: if it is not xssh itself, kill the foreground process and print "-xssh: Exit pid childpid", where childpid is the pid of the current process
	//hint: remember to put the code "fflush(stdout);" after printing the message above for a clear output
	printf("Replace me for ctrl+C handler\n");
}

/*wait instruction*/
void waitchild(char buffer[BUFLEN])
{
	int i;
	int start = 5;

	/*store the childpid in pid*/
	char number[BUFLEN] = {'\0'}; 
	while(buffer[start]==' ')start++;
	for(i = start; (i < strlen(buffer))&&(buffer[i]!='\n')&&(buffer[i]!='#'); i++)
	{
		number[i-start] = buffer[i];
	}
        rootdir[i-start] = '\0';
	char *endptr;
	int pid = strtol(number, &endptr, 10);

	/*simple check to see if the input is valid or not*/
	if((*number != '\0')&&(*endptr == '\0'))
	{
		//FIXME: if pid is not -1, try to wait the background process pid
		//FIXME: if successful, print "-xssh: Have finished waiting process pid", where pid is the pid of the background process
		//FIXME: if not successful, print "-xssh: Unsuccessfully wait the background process pid", where pid is the pid of the background process


		//FIXME: if pid is -1, print "-xssh: wait childnum background processes" where childnum stores the number of background processes, and wait all the background processes
		//hint: remember to set the childnum correctly after waiting!
		printf("Replace me for wait P\n");

	}
	else printf("-xssh: wait: Invalid pid\n");
}

/*execute the external command*/
int program(char buffer[BUFLEN])
{
	/*if backflag == 0, xssh need to wait for the external command to complete*/
	/*if backflag == 1, xssh need to execute the external command in the background*/
	int backflag = 0;
	char *ptr = strchr(buffer, '&');
	if(ptr != NULL) backflag = 1;

	pid_t pid;
	//FIXME: create a new process for executing the external command

	//FIXME: remember to check if the process creation is successful or not. if not, print error message and return -2, see codes below;


	//FIXME: write the code to execute the external command in the newly created process, using execvp()
	//hint: the external command is stored in buffer, but before execute it you may need to do some basic validation check or minor changes, depending on how you execute
	//FIXME: remember to check if the external command is executed successfully; if not, print error message "-xssh: Unable to execute the instruction buffer", where buffer is replaced with the actual external command to be printed
	//hint: after executing the extenal command using execvp(), you need to return -1;
	/*for extra credit, implement stdin/stdout redirection in here*/

	printf("Replace me for executing external commands\n");

	//FIXME: in the xssh process, remember to act differently, based on whether backflag is 0 or 1
	//hint: the codes below are necessary to support command "wait -1", but you need to put them in the current place
		childnum++;
			childnum--; //this may or may not be needed, depending on where you put the previous line
	//hint: the code below is necessary to support command "show $!", but you need to put it in the current place
			sprintf(varvalue[2], "%d\0", pid);
			return 0;
}

/*for extra credit, implement the function below*/
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


