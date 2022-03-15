#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <map>
#include <ctime>
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

/*
 NOTE: the enviorment the shell supports is Ubuntu 18.04 LTS
  Basic Shell that supports the following commands:
    Built-in Commands:
      1. chprompt <input> - allowing the user to the change the prompt to a string of their choice, if not specified the prompt will reset (smash by default)
      2. showpid - prints the PID of the shell (current process running)
      3. pwd - prints the path of the current directory
      4. cd <input> - change directory to the given path (special arguments: '-' last working directory)
      5. jobs command - in addition to the pid each process running in the background is assigned with a jobid 
                        this command prints the josb list (unfinished and stopped)
      6. kill -<signal> <jobid> - sends a signal with the value of signal to the job with the specified job id 
      7. fg <jobid> - brings a stopped process or background running process (with the specified jobid) to the foreground 
      8. bg <jobid> - resumes a stopped process in the background
      9. quit [kil] - exits the shell (optional:specifing kill argument will kill all unfinished and stopped processes)
    10. External Commands - shell supports bash commands for all the none built in commands
    11. head [-N] <file> - the command prints the first N line of the file, if N is not specified its 10 y default
    #The shell supports Pipes using command1 | command2 - redirects command1 stdout to its write channel and command2 stdin to its read
                                  channel
                                  command1 |& command 2 - redirects command1 stderr to the pipe’s write channel and command2 stdin
                                  to the pipe’s read channel
    #The shell supports redirection using command > output redirects the output of the command to the specified file. If the file exists it overrides it 
                                        command >> output same, but if the file exists it will append to the file
    #External commands can run in the background if you add & to the end of the command line
*/ 
class Command {
  protected:
  const std::vector<std::string>& cmd_vec_m;
  const std::string command_line;

  public:
  Command(std::vector<std::string>& cmd_vec, std::string cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  const std::string getCommandLine();
  const std::string getCommandLine() const;
  protected:
  static bool _isStringDigit(std::string s);
  // virtual void prepare();
  // virtual void cleanup();
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(std::vector<std::string>& cmd_vec, std::string cmd_line);
  virtual ~BuiltInCommand() {}
};

class PipeCommand : public Command {
  bool is_std_err;
  Command* command_left;
  Command* command_right;
  
 public:
  PipeCommand(std::vector<std::string>& cmd_vec, std::string cmd_line);
  virtual ~PipeCommand();
  void execute() override;
};

class RedirectionCommand : public Command {
 bool is_append;
 std::string file_name;
 Command* redirected_command;

 public:
  explicit RedirectionCommand(std::vector<std::string>& cmd_vec, std::string cmd_line);
  virtual ~RedirectionCommand();
  void execute() override;

};

class ChangePromptCommand : public BuiltInCommand {
std::string prompt;
public:

  ChangePromptCommand(std::vector<std::string>& cmd_vec, std::string cmd_line);
  virtual ~ChangePromptCommand() {}
  void execute() override;
  std::string getPrompt();
};

class ChangeDirCommand : public BuiltInCommand {
  std::string last_pwd;
  public:
  ChangeDirCommand(std::vector<std::string>& cmd_vec, std::string cmd_line, std::string plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
  void checkStatus(int status,char* curr_path);
  void goBack(char* curr_path);
  void goTo(const char* path, char* curr_path);
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(std::vector<std::string>& cmd_vec, std::string cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(std::vector<std::string>& cmd_vec, std::string cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList {
 public:
  class JobEntry {
   Command* command;
   int job_id;
   pid_t job_pid;
   time_t insert_time;
   bool stopped;

   public:
   JobEntry(Command* cmd, int id, pid_t pid, bool stop_flag = false);
   ~JobEntry() = default;
   void stopJob();
   void resumeJob();
   void printJobEntry() const;
   bool isStopped()
   {return stopped;}
   const Command* getCommand(){return command;}
   const Command* getCommand() const{return command;}
   int getJobid() const {return job_id;}
   pid_t getJobPid() const {return job_pid;}
   void changeJobId(int new_job_id) {job_id = new_job_id;}
   
  };
 int max_id;
 int running_counter;
 std::map<int, JobEntry> running_jobs_list;
 std::map<int, JobEntry> stopped_jobs_list;
 static const int max_processes = 100;
 
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd,pid_t pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  std::map<int,JobEntry>::iterator getJobById(int jobId, bool& exists);
  std::map<int,JobEntry>::iterator getJobByPid(int jobPid, bool& exists);
  void removeJobById(int jobId);
  JobEntry *getLastJob(int* lastJobId);
  JobEntry *getLastRunningJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  void updateMaxID();
  bool isEmpty()
  {return running_jobs_list.empty() && stopped_jobs_list.empty();}
  bool isStoppedListEmpty()
  {return stopped_jobs_list.empty();}
  void resumeJob(int job_id);
  void recoverOldJobId(pid_t job_pid,int job_id);
};

class ExternalCommand : public Command {
  JobsList& jobs_list;
 public:
  ExternalCommand(std::vector<std::string>& cmd_vec, std::string cmd_line, JobsList& jobs);
  virtual ~ExternalCommand() {}
  void execute() override;
  void initArgv(std::vector<char*>&, std::string&);
};


class QuitCommand : public BuiltInCommand {
  JobsList& jobs_list;
  public:
  QuitCommand(std::vector<std::string>& cmd_vec, std::string cmd_line, JobsList& jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobsCommand : public BuiltInCommand {
 JobsList& jobs_list;
 public:
  JobsCommand(std::vector<std::string>& cmd_vec,std::string cmd_line, JobsList& jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 JobsList& jobs_list;
 int signum;
 int job_id;
 public:
  KillCommand(std::vector<std::string>& cmd_vec,std::string cmd_line, JobsList& jobs);
  virtual ~KillCommand() {}
  void execute() override;
  bool isSignalValid();
  bool isJobIdValid();

};

class ForegroundCommand : public BuiltInCommand {
 JobsList& jobs_list;
 public:
  ForegroundCommand(std::vector<std::string>& cmd_vec,std::string cmd_line, JobsList& jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 JobsList& jobs_list;
 public:
  BackgroundCommand(std::vector<std::string>& cmd_vece, std::string cmd_line,JobsList& jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class HeadCommand : public BuiltInCommand {
  std::string file_name;
  int num_of_lines;

 public:
  HeadCommand(std::vector<std::string>& cmd_vec,std::string cmd_line);
  virtual ~HeadCommand() {}
  void execute() override;
  static bool getN(std::vector<std::string>& cmd_vec,int& n);
  static void getFileName(std::vector<std::string>& cmd_vec, std::string& file_name, bool is_n_specified);
  static int readLine(int fd, std::string& line);
  static int writeLine(int fd, std::string& line);
};


class SmallShell {
 private:
  JobsList* jobs;
  SmallShell();
  pid_t smash_pid;
  std::string prompt;
  std::string prev_path;
  pid_t foreground_pid;
  Command* foreground_command;
  bool is_pipe;
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  void printPrompt();
  void setPrompt(std::string new_prompt);
  void updateForegroundCommand(Command* new_command, pid_t new_pid);
  pid_t sendSignalToForgroundCommand(int signal);
  void updatePrevPath(std::string& new_prev_path);
  pid_t getSmashPid(){return smash_pid;}
  void updatePipe(bool pipe){is_pipe = pipe;}
  bool checkPipe(){return is_pipe;}

  friend class CommandFactory;

};
  class CommandFactory{
  public:
  CommandFactory();
  static Command* getCommand(const char* cmd_line, SmallShell& shell );
  static void checkRedirectedOrPipe(const char* cmd_line, bool&  redirected, bool& pipe);
  };


#endif //SMASH_COMMAND_H_
