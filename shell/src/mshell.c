#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "config.h"
#include "siparse.h"
#include "backgrounds.h"
#include "builtins.h"


char buf[MAX_LINE_LENGTH + 1];
char line[MAX_LINE_LENGTH + 1];
int pipefd[2];
int pom_pipefd;



void
print_exec_errors(char * first_argument) {
	fprintf(stderr, "%s", first_argument);

	if (errno == ENOENT) {
		fprintf(stderr, ": no such file or directory\n");
	}
	else if(errno == EACCES) {
		fprintf(stderr, ": permission denied\n");
	}
	else {
		fprintf(stderr, ": exec error\n");
	}
}





void
handle_redirs(redir * red) {
	int op;
	if (IS_RIN(red->flags)) {
		close(0);
		op = open(red->filename, O_RDONLY);

		if (op == -1) {
			if (errno == ENOENT) {
				fprintf(stderr, "%s: no such file or directory\n", red->filename);
				exit(EXIT_FAILURE);
			}
			if(errno == EACCES) {
				fprintf(stderr, "%s: permission denied\n", red->filename);
				exit(EXIT_FAILURE);

			}
		}
	}

	if (IS_ROUT(red->flags) || IS_RAPPEND(red->flags)) {
		if (IS_RAPPEND(red->flags)) {
			close(1);
			op = open(red->filename, O_APPEND | O_WRONLY, 0644);
		}
		else {
			close(1);
			op = open(red->filename, O_TRUNC | O_WRONLY, 0644);

		}

		if (op == -1) {
			if (errno == ENOENT) {
				op = open(red->filename, O_CREAT | O_WRONLY, 0644);
				if (op == -1) {
					fprintf(stderr, "%s: file create error\n", red->filename);
					exit(EXIT_FAILURE);
				}
			}
			else if(errno == EACCES) {
				fprintf(stderr, "%s: permission denied\n", red->filename);
				exit(EXIT_FAILURE);
			}
		}
	}



}



void
handle_child_before_exec(command * com, int is_background, int comseq_index, int comseq_number) {

	if (is_background) {
		setsid();
	}

	if (comseq_index > 0) {
		int dp2 = dup2(pom_pipefd, 0);
		if (dp2 == -1) {
			fprintf(stderr, "dup2 error\n");
			exit(EXIT_FAILURE);
		}
		close(pom_pipefd);

	}


	if (comseq_index < comseq_number - 1) {
		int dp2  = dup2(pipefd[1], 1);
		if (dp2 == -1) {
			fprintf(stderr, "dup2 error\n");
			exit(EXIT_FAILURE);
		}

		close(pipefd[1]);
		close(pipefd[0]);
	}

	redirseq * redseq = com -> redirs;
	redirseq * redseqpom = redseq;

	if (redseq != NULL) {
		do  {
			redir * red = redseq -> r;
			handle_redirs(red);

			redseq = redseq -> next;

		} while(redseq != redseqpom);

	}


}

sigset_t sigsuspendmask;
sigset_t childblockmask;


int
main(int argc, char *argv[])
{

	pipelineseq * ln;
	ssize_t r = 0;
	pid_t k;
	int x;
	struct stat st;
	int line_end = 0;
	int is_eof = 0;
	int buf_start = 0;
	int buf_end = 0;
	int buf_len = 0;
	int full_line = 1;
	int ignore_line = 0;
	backgrounds_count = 0;
	foregrounds_count = 0;
	status_count = 0;


	struct sigaction sa;
	sa.sa_handler = child_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGCHLD, &sa, NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa, NULL);

	sigemptyset(&childblockmask);
	sigaddset(&childblockmask, SIGCHLD);
	sigemptyset(&sigsuspendmask);

	while (1) {

		//start od input handling

		if (is_eof == 1) {
			break;
		}

		if (full_line == 1) {
			int fs = fstat(0, &st);
			if (fs == -1) {
				fprintf(stderr, "fstat error\n");
				exit(EXIT_FAILURE);
			}
			if (S_ISCHR(st.st_mode)) {
				write_statuses();
				write(1,PROMPT_STR,strlen(PROMPT_STR));
			}

		}
		if (buf_start >= r) {
			while (1) {
				r = read(0, buf,MAX_LINE_LENGTH);
				if (r == 0) {
					exit(0);
				}
				if (r == -1) {
					if (errno == EINTR) {
						continue;
					}
					fprintf(stderr, "read error\n");
					exit(EXIT_FAILURE);
				}
				buf_start = 0;
				break;
			}
		}

		char * startpointer = buf + buf_start;
		char * endpointer = buf + r;
		char * newfile = strchr(startpointer, '\0');
		char * newline = strchr(startpointer, '\n');

		if (newline == NULL || newline >= endpointer ||  (newfile != NULL && newfile < newline) ) {
			newline = newfile;
		}
		if (newline != NULL && newline < endpointer) {
			buf_end = (int)(newline - buf);
			if (*newline == '\0') {
				is_eof = 1;
			}
			full_line = 1;
		}
		else {
			buf_end = (int)(r - 1);
			full_line = 0;
		}

		buf_len = buf_end - buf_start + 1;

		if (ignore_line == 0) {

			if (line_end + buf_len > MAX_LINE_LENGTH) {
				fprintf(stderr, SYNTAX_ERROR_STR);
				fprintf(stderr, "\n");
				ignore_line = 1;
			}
		}

		if (ignore_line == 0) {
			memcpy(line + line_end, startpointer, buf_len - 1 );
		}

		line_end = line_end + buf_len - 1;
		buf_start = buf_end + 1;
		if (full_line == 1) {

			if (ignore_line == 1) {
				ignore_line = 0;
				line_end = 0;
				continue;
			}
			line[line_end] = '\0';
			line_end = 0;
		}
		else {
			if (ignore_line == 1) {
				line_end = 1;
			}
			else {
				line[line_end] = buf[buf_end];
				line_end++;
			}
			continue;
		}

		//end of input handling

		ln = parseline(line);

		if (ln == NULL) {
			fprintf(stderr, SYNTAX_ERROR_STR);
			fprintf(stderr, "\n");
			continue;
		}
			pipelineseq * ppls = ln;
			pipelineseq * pom_ppls = ppls;

			do {
				pipeline * ppline = ppls -> pipeline;
				commandseq * comseq = ppline -> commands;
				commandseq * pom_comseq = comseq;
				int comseq_number = 0;
				int is_null_command = 0;
				int is_background = 0;

				if (ppline -> flags & INBACKGROUND) {
					is_background = 1;
				}

				if (comseq == NULL) {
					ppls = ppls -> next;
					continue;
				}
				while (pom_comseq -> next != comseq) {
					comseq_number++;
					pom_comseq = pom_comseq -> next;
					if (pom_comseq -> com == NULL) {
						is_null_command = 1;
					}
				}

				if (pom_comseq -> com == NULL) {
					is_null_command = 1;
				}
				comseq_number++;
				if (is_null_command == 1 && comseq_number > 1) {
					fprintf(stderr, SYNTAX_ERROR_STR);
					fprintf(stderr, "\n");
					break;
				}

				for (int comseq_index = 0; comseq_index < comseq_number; comseq_index++) {

					if (comseq_index < comseq_number - 1 ) {
						int pp = pipe(pipefd);
						if (pp == -1) {
							fprintf(stderr, "pipe error\n");
							exit(EXIT_FAILURE);
						}
					}
					command * com = comseq -> com;
					if (com == NULL) {
						continue;
					}
					argseq * seq = com->args;
					argseq * seqpom = seq;
					int arg_number = 0;
					while (seqpom->next != seq) {
						arg_number++;
						seqpom = seqpom->next;
					}
					seqpom = seqpom->next;
					arg_number++;
					char * arguments[arg_number+1];
					int seq_index  = 0;
					while (seq->next != seqpom) {
						arguments[seq_index] = seq->arg;
						seq = seq->next;
						seq_index++;
					}

					if (arg_number > 0) {
						arguments[arg_number-1] = seq->arg;
					}

					arguments[arg_number] = NULL;
					if (comseq_number == 1) {
						if (is_builtin(arguments)) {
							continue;
						}
					}

					sigprocmask(SIG_BLOCK, &childblockmask, NULL);

					k = fork();

					if (k == -1) {
						fprintf(stderr, "fork error\n");
						exit(EXIT_FAILURE);
					}

					if (k == 0) {

						handle_child_before_exec(com, is_background, comseq_index, comseq_number);

						sa.sa_handler = SIG_DFL;
						sigaction(SIGINT, &sa, NULL);

						x = execvp(arguments[0], arguments);
						if (x == -1) {
							print_exec_errors(arguments[0]);
							exit(EXEC_FAILURE);
						}
					}
					else { //parent
						if (is_background) {
							add_background(k);
						}
						else {
							foregrounds_count++;
						}
						if (comseq_index > 0) {
							close(pom_pipefd);
						}
						if (comseq_index < comseq_number - 1) {
							close(pipefd[1]);
						}
						pom_pipefd = pipefd[0];
						comseq = comseq -> next;

					}

				}
				while (foregrounds_count > 0) {
					sigsuspend(&sigsuspendmask);
				}
				sigprocmask(SIG_UNBLOCK, &childblockmask, NULL);

				ppls = ppls -> next;
			} while (ppls != pom_ppls);
	}

	return 0;

}
