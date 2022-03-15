# Small-Shell

NOTE: the enviorment the shell supports is Ubuntu 18.04 LTS
Basic Shell that supports the following commands:
Built-in Commands:
1. **chprompt <input>** - allowing the user to the change the prompt to a string of their choice, if not specified the prompt will reset (smash by default)
1. **showpid**- prints the PID of the shell (current process running)
1. **pwd** - prints the path of the current directory
1. **cd <input>** - change directory to the given path (special arguments: '-' last working directory)
1. **jobs command** - in addition to the pid each process running in the background is assigned with a jobid 
this command prints the josb list (unfinished and stopped)
1. **kill -<signal> <jobid>** - sends a signal with the value of signal to the job with the specified job id 
1. **fg <jobid>** - brings a stopped process or background running process (with the specified jobid) to the foreground 
1. **bg <jobid>** - resumes a stopped process in the background
1. **quit [kil]** - exits the shell (optional:specifing kill argument will kill all unfinished and stopped processes)
1. **External Commands** - shell supports bash commands for all the none built in commands
1. **head [-N] <file>** - the command prints the first N line of the file, if N is not specified its 10 by default.
 
- The shell supports Pipes using 
  * command1 | command2 - redirects command1 stdout to its write channel and command2 stdin to its read channel
  * command1 |& command 2 - redirects command1 stderr to the pipe’s write channel and command2 stdin to the pipe’s read channel
- The shell supports redirection using 
  * command > output redirects the output of the command to the specified file. If the file exists it overrides it 
  * command >> output same, but if the file exists it will append to the file
 
 
External commands can run in the background if you add & to the end of the command line
