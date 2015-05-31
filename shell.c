#include<unistd.h>
#include<errno.h>
#include<linux/limits.h>
#include"string.h"

#define SHELL_NAME "Shell"
#define CMDLINE_BUFF_SIZE 1024 * 16
#define CMD_ARGUMENT_MAX 256

char cmdline[CMDLINE_BUFF_SIZE];
size_t len_cmdline;
const char *path = NULL;//the first parameter of execvp
char * arg_list[CMD_ARGUMENT_MAX + 1];//the second parameter of execvp

char current_directory[PATH_MAX + 1];
int is_running = 1;

void warning(const char *msg) {
	write(2, SHELL_NAME ": ", sizeof(SHELL_NAME ": ") - 1);
	if (msg) {
		write(2, msg, strlen(msg));
		write(2, "\n", 1);
	} else {
		write(2, "Internal Error\n", 15);
	}
}
void fault(const char *msg) {
	warning(msg);
	_exit(1);
}

ssize_t prompt(void) {
	char *cwd = getcwd(current_directory, sizeof(current_directory));
	if (cwd)
		write(2, cwd, strlen(cwd));
	if (getuid() == 0) {
		return write(2, " #", 2);
	} else {
		return write(2, " $", 2);
	}
}

ssize_t read_cmdline(void) {
	ssize_t ret = read(0, cmdline, sizeof(cmdline));
	if (ret >= 0) len_cmdline = ret;
	return ret;
}

int parse(void) {
	if (len_cmdline == 0)
		return 0;
	unsigned arg_i = 0;
	char *cmdtail = cmdline + len_cmdline,
			   *scaner = cmdline,
			   *word = cmdline,
			   *printer = cmdline;
	char status = 0;
	
	while (scaner != cmdtail) {
		switch (*scaner) {
			case '"':
				if (status == '"') {
					status = 0;
				} else if (status == 0) {
					status = '"';
				} else {
					break;
				}
				scaner++;
				continue;
			case '\'':
				if (status == '\'') {
					status = 0;
				} else if (status == 0) {
					status = '\'';
				} else {
					break;
				}
				scaner++;
				continue;
			case ' ':
			case '\n':
			case '\r':
				if (status == 0) {
					if (word == printer ) {
						word++;
						printer++;
					} else {
						*printer = 0;
						if (!path) {
							path = word;
						} 
						arg_list[arg_i++] = word;
						printer++;
						word = printer;
						if (arg_i >= CMD_ARGUMENT_MAX)
							goto out;
					}
					scaner++;
					continue;
				}
		}
		*printer++ = *scaner++;
	}
	if (status != 0) {
		//Quote Problem
	}
	if (word != printer ) {
		*printer = 0;
		if (!path) {
			path = word;
		} 
		arg_list[arg_i++] = word;
	}
out:
	arg_list[arg_i] = 0;
	return arg_i;
}

int buildin(void) {
	if (strncmp(path, "exit", 5) == 0) {
		is_running = 0;
		return 0;
	} else if (strncmp(path, "cd", 3) == 0) {
		int ret = 1;
		if (arg_list[1]) {
			ret = chdir(arg_list[1]);
		} else {
			warn("Argument required");
		}
		return ret;
	} else {
		return -1;
	}
}

int shell(void) {
	pid_t pid;
	pid = vfork();
	if (pid == 0) {
		execvp(path, arg_list);
		fault(strerror(errno));
	} else if (pid > 0) {
		int ret = 1;
		pid_t result = waitpid(pid, &ret, 0);
		if (result == pid) {
			return ret;
		}
	}
	warning(strerror(errno));
	return 1;
}

#ifdef BUILD_NOSTDLIB
void __start(void) {
#else
int main(int argc, char **argv) {
#endif
	int ret;
	while(is_running) {
		if (prompt() == -1) {
			fault("write prompt FAIL"); //Is there any problem ? :<
		}
		if (read_cmdline() == -1) {
			fault("read cmdline FAIL");
		}
		if (parse() != 0) {
			if (buildin() == -1)
				shell();
		}
	}
	_exit(ret);
}
