#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

void test_cmd_node(struct cmd_node *p) {
	fprintf(stderr, "===== cmd_node =====\n");
	for(int i=0; p->args[i] != NULL; i++) {
		fprintf(stderr, "args[%d]: %s ", i, p->args[i]);
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "length: %d\n", p->length);
	fprintf(stderr, "in_file: %s\n", p->in_file);
	fprintf(stderr, "out_file: %s\n", p->out_file);
	fprintf(stderr, "in: %d\n", p->in);
	fprintf(stderr, "out: %d\n", p->out);
	fprintf(stderr, "====================\n");
	return;
}

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p){
	// <
	if(p->in_file) {
		p->in = 3;
		int fd = open(p->in_file, O_RDONLY);
		if(fd == -1) {
			perror("open");
			exit(EXIT_FAILURE);
		}
		if(dup2(fd, 0) == -1) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	// >
	if(p->out_file) {
		p->out = 4;
		int fd = open(p->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(fd == -1) {
			perror("open");
			exit(EXIT_FAILURE);
		}
		if(dup2(fd, 1) == -1) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	return;
}
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p)
{
	pid_t pid;
	int status;
	pid = fork();
	if(pid == 0) {
		redirection(p);
		if(execvp(p->args[0], p->args) == -1){
			fprintf(stderr, "execvp error\n");
			exit(EXIT_FAILURE);
		}
	}
	else if(pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	waitpid(pid, &status, 0);
  	return 1;
}
// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int fork_cmd_node(struct cmd *cmd)
{
	int status;
	struct cmd_node *curr = cmd->head;
	int cmd_count = 0;
	while(curr) {
		cmd_count++;
		curr = curr->next;
	}

	int pipes[cmd_count-1][2];
	for(int i=0; i<cmd_count-1; i++) {
		if(pipe(pipes[i]) == -1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}
	}

	curr = cmd->head;
	pid_t pid;
	int cmd_index = 0;
	while(curr) {
		pid = fork();
		if(pid == 0) {
			//first cmd
			if(cmd_index == 0) {
				close(pipes[cmd_index][0]);
				if(curr->next != NULL) {
					if(dup2(pipes[cmd_index][1], 1) == -1) {
						perror("dup2");
						exit(EXIT_FAILURE);
					}
				}
			}
			//last cmd
			else if(cmd_index == cmd_count-1) {
				close(pipes[cmd_index-1][1]);
				if(dup2(pipes[cmd_index-1][0], 0) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}
			else {
				if(dup2(pipes[cmd_index-1][0], 0) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
				if(dup2(pipes[cmd_index][1], 1) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}
			for(int i=0; i<cmd_count-1; i++) {
				close(pipes[i][0]);
				close(pipes[i][1]);
			}
			redirection(curr);
			test_cmd_node(curr);
			if(execvp(curr->args[0], curr->args) == -1) {
				perror("execvp");
				exit(EXIT_FAILURE);
			}
		}
		else if(pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}

		cmd_index++;
		curr = curr->next;
	}

	for(int i=0; i<cmd_count-1; i++) {
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
	for(int i=0; i<cmd_count; i++) {
		wait(&status);
	}
	return 1;
}
// ===============================================================


void shell()
{
	while (1) {
		printf(">>> $ ");
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);
		
		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;
		
		if(temp->next == NULL){
			status = searchBuiltInCommand(temp);
			if (status != -1){
				int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
				if( in == -1 || out == -1)
					perror("dup");
				redirection(temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (temp->in_file)  dup2(in, 0);
				if (temp->out_file){
					dup2(out, 1);
				}
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(cmd->head);
			}
		}
		// There are multiple commands ( | )
		else{
			
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			
			struct cmd_node *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
