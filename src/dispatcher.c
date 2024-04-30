#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dispatcher.h"
#include "shell_builtins.h"
#include "parser.h"

static int handleProcess(struct command *pipeline); 

/* 
 * function to handle pipeline functionality
 * Argument: pipeline
 * Returns: 0 if pipe execution was successful
 */
static int handlePipe(struct command *pipeline)
{
        int pid1;
        int pid2;       
        int fd[2];
        int status1;
        int status2;
        
        if (pipe(fd) == -1){
                fprintf(stderr, "Pipe failed");
                exit(EXIT_FAILURE);
        }

        if ((pid1 = fork()) < 0) {
                fprintf(stderr, "First fork failed\n");
                exit(EXIT_FAILURE);
        }

        // child 1
        else if (pid1 == 0){   
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);  
                
                execvp(pipeline->argv[0], pipeline->argv);
                fprintf(stderr, "%s: command not found.\n", pipeline->argv[0]);
                exit(EXIT_FAILURE);
        }
        
        // parent 1
        else {
                close(fd[1]); 
                
                // wait for child 1 
                if (waitpid(pid1, &status1, 0) > 0){

                        if ( WEXITSTATUS(status1) != EXIT_SUCCESS)
                                return -1;
                }
        
                else {
                        fprintf(stderr, "Wait failed\n");
                        exit(EXIT_FAILURE);
                }

                
                if ((pid2 = fork()) < 0) {
                        fprintf(stderr, "Second fork failed\n");
                        exit(EXIT_FAILURE);
                }
                
                // child 2
                else if (pid2 == 0) {
                        dup2(fd[0], STDIN_FILENO);
                
                        if (pipeline->pipe_to->output_type != COMMAND_OUTPUT_STDOUT){
                                if (handleProcess(pipeline->pipe_to) != 0)
                                        return -1;
                        }

                        execvp(pipeline->pipe_to->argv[0], pipeline->pipe_to->argv);
                        fprintf(stderr, "%s: command not found.\n", pipeline->pipe_to->argv[0]);
                        exit(EXIT_FAILURE);
                }

                
                // parent 2
                else {
                        close(fd[0]);
                        
                        // wait for child 2
                        if (waitpid(pid2, &status2, 0) > 0){
                                
                                if (WEXITSTATUS(status2) != EXIT_SUCCESS)
                                        return -1;
                        }
        
                        else {
                                fprintf(stderr, "Wait failed\n");
                                exit(EXIT_FAILURE);
                        }  
                        
                        exit(EXIT_SUCCESS);     

                }
                exit(EXIT_SUCCESS);
        }

        return 0;
}

/*
 * Function to handle pipline output types
 * Argument: pipeline (A "struct command" pointer representing one or more
 *      commands chained together in a pipeline)
 * Returns: the file descriptor resulting from the various pipeline output types
 */

static int handleProcess(struct command *pipeline) 
{
        int fileDescriptor = 0;
        
        switch (pipeline->output_type) {
        case COMMAND_OUTPUT_FILE_APPEND:
                fileDescriptor = open(pipeline->output_filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
                if (fileDescriptor < 0) 
                        return -1;
                
                dup2(fileDescriptor, 1);
                        
                return 0;
                break;

        case COMMAND_OUTPUT_FILE_TRUNCATE:
                fileDescriptor = open(pipeline->output_filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);

                if (fileDescriptor < 0) 
                        return -1;
        
                dup2(fileDescriptor, 1);
                        
                return 0;
                break;

        case COMMAND_OUTPUT_PIPE:
                if (handlePipe(pipeline) != 0)
                        return -1;

                return 0;
                break;

        default:
                if (pipeline->input_filename == NULL)
                        return 0;

                fileDescriptor = open(pipeline->input_filename, O_RDONLY);
                         
                if (fileDescriptor >= 0) {
                        dup2(fileDescriptor, 0);
                        return 0;
                }

                else {
                        return -1;
                }
                break;
        }
}





/**
 * dispatch_external_command() - run a pipeline of commands
 *
 * @pipeline:   A "struct command" pointer representing one or more
 *              commands chained together in a pipeline.  See the
 *              documentation in parser.h for the layout of this data
 *              structure.  It is also recommended that you use the
 *              "parseview" demo program included in this project to
 *              observe the layout of this structure for a variety of
 *              inputs.
 *
 * Note: this function should not return until all commands in the
 * pipeline have completed their execution.
 *
 * Return: The return status of the last command executed in the
 * pipeline.
 */
static int dispatch_external_command(struct command *pipeline)
{
	/*
	 * Note: this is where you'll start implementing the project.
	 *
	 * It's the only function with a "TODO".  However, if you try
	 * and squeeze your entire external command logic into a
	 * single routine with no helper functions, you'll quickly
	 * find your code becomes sloppy and unmaintainable.
	 *
	 * It's up to *you* to structure your software cleanly.  Write
	 * plenty of helper functions, and even start making yourself
	 * new files if you need.
	 *
	 * For D1: you only need to support running a single command
	 * (not a chain of commands in a pipeline), with no input or
	 * output files (output to stdout only).  In other words, you
	 * may live with the assumption that the "input_file" field in
	 * the pipeline struct you are given is NULL, and that
	 * "output_type" will always be COMMAND_OUTPUT_STDOUT.
	 *
	 * For D2: you'll extend this function to support input and
	 * output files, as well as pipeline functionality.
	 *
	 * Good luck!
	 */
        
        int pid = fork();
        int fileDescriptor;
        int status;

        if (pid < 0) {
                fprintf(stderr, "fork failed\n");
                exit(EXIT_FAILURE);
        }
        
        // child
        else if (pid == 0){
                
                if ((fileDescriptor = handleProcess(pipeline)) != 0)
                        exit(EXIT_FAILURE);
                
                if (pipeline->output_type != COMMAND_OUTPUT_PIPE) {
                        execvp(pipeline->argv[0], pipeline->argv);
                        fprintf(stderr, "%s: command not found.\n", pipeline->argv[0]);
                        exit(EXIT_FAILURE);
                }
        }
        
        // parent
        else{
                // wait for child
                if (waitpid(pid, &status, 0) > 0){

                        if ( WEXITSTATUS(status) != EXIT_SUCCESS ) {
                                return -1;
                        }
                        
                }
        
                else {
                        fprintf(stderr, "Wait failed\n");
                        exit(EXIT_FAILURE);
                }
                //exit(EXIT_SUCCESS);

        }
	return 0;
}

/**
 * dispatch_parsed_command() - run a command after it has been parsed
 *
 * @cmd:                The parsed command.
 * @last_rv:            The return code of the previously executed
 *                      command.
 * @shell_should_exit:  Output parameter which is set to true when the
 *                      shell is intended to exit.
 *
 * Return: the return status of the command.
 */
static int dispatch_parsed_command(struct command *cmd, int last_rv,
				   bool *shell_should_exit)
{
	/* First, try to see if it's a builtin. */
	for (size_t i = 0; builtin_commands[i].name; i++) {
		if (!strcmp(builtin_commands[i].name, cmd->argv[0])) {
			/* We found a match!  Run it. */
			return builtin_commands[i].handler(
				(const char *const *)cmd->argv, last_rv,
				shell_should_exit);
		}
	}

	/* Otherwise, it's an external command. */
	return dispatch_external_command(cmd);
}

int shell_command_dispatcher(const char *input, int last_rv,
			     bool *shell_should_exit)
{
	int rv;
	struct command *parse_result;
	enum parse_error parse_error = parse_input(input, &parse_result);

	if (parse_error) {
		fprintf(stderr, "Input parse error: %s\n",
			parse_error_str[parse_error]);
		return -1;
	}

	/* Empty line */
	if (!parse_result)
		return last_rv;

	rv = dispatch_parsed_command(parse_result, last_rv, shell_should_exit);
	free_parse_result(parse_result);
	return rv;
}
