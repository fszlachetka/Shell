#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#include "builtins.h"

#include <ctype.h>
#include <signal.h>

int echo(char * []);
int undefined(char * []);
int lexit(char * []);
int lcd(char * []);
int lkill(char * []);
int lls(char * []);


builtin_pair builtins_table[]={
	{"exit",	&lexit},
	{"lecho",	&echo},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
	{NULL,NULL}
};


int
is_builtin(char * arguments[]) {
	int index = 0;
	while (builtins_table[index].name != NULL) {
		if (strcmp(arguments[0], builtins_table[index].name) == 0) {
			if (builtins_table[index].fun(arguments) == BUILTIN_ERROR) {
				fprintf(stderr, "Builtin %s error.\n", arguments[0]);
			}
			return 1;
		}
		index++;
	}

	return 0;

}

pid_t
conversion(const char * string) {
	pid_t number = 0;

	for (int i = 0; string[i] != '\0'; i++) {
		if (string[i] < '0' || string[i] > '9') {
			return -1;
		}
		number = number * 10 + (string[i] - '0');
	}
	return number;
}


int 
echo( char * argv[])
{
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int 
undefined(char * argv[])
{
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}


int
lexit(char * argv[]) {
	exit(0);
}


int
lcd(char * argv[]) {

	if (argv[1] == NULL) {
		if (chdir(getenv("HOME")) == 0)
			return 0;
		return BUILTIN_ERROR;
	}

	if (argv[2] != NULL) {
		return BUILTIN_ERROR;
	}

	if (chdir(argv[1]) == 0) {
		return 0;
	}

	return BUILTIN_ERROR;
}



int
lkill (char * argv[]) {
	if (argv[1] == NULL) {
		return BUILTIN_ERROR;
	}
	if (argv[2] == NULL) {
		int pid_number = conversion(argv[1]);
		if (pid_number != -1) {
			if (kill(pid_number, SIGTERM) == 0) {
				return 0;
			}
		}
		return BUILTIN_ERROR;
	}
	char * signal_string = argv[1];
	if (signal_string[0] == '-') {

		signal_string ++;
		int pid_number = conversion(argv[2]);
		int sig_number = conversion(signal_string);

		if (pid_number != -1 && sig_number != -1) {
			if (kill(pid_number, sig_number) == 0) {
				return 0;
			}
		}
		return BUILTIN_ERROR;
	}

	return BUILTIN_ERROR;

}

int lls (char * argv[]) {

	DIR * dir = opendir(".");
	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] != '.') {
			printf("%s\n", entry->d_name);
		}
	}
	closedir(dir);
	fflush(stdout);
	return 0;
}

