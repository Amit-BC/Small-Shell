#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <signal.h>
#include <utility>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << S" <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define FAIL -1
#define SUCCESS 0 
#define EOF_REACHED 1
#define MAX_ARGS 21

const std::string WHITESPACE = " \n\r\t\f\v";

/* Auxiliary Functions */

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == std::string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void _removeBackgroundSignString(std::string& string)
{ 
  // find last character other than spaces
  size_t idx = string.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (string[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  string.erase(idx, 1);
}

std::vector<std::string>* _createCommandVector(const char* cmd_line)
{
  if(cmd_line == NULL){return nullptr;}
	std::vector<std::string>* cmd_vector = new std::vector<std::string>;
	char** args = new char*[MAX_ARGS];
	int size = _parseCommandLine(cmd_line, args);
	for(int i = 0; i < size; i++)
	{
		cmd_vector->push_back(string(args[i]));
		free(args[i]); 
	}
	delete[] args;
	return cmd_vector;
}

void _getSubStrings(const std::string& cmd_line, std::string& left_substring, std::string&  right_substring, 
                          char first_char, char second_char, bool& second_char_exists)
{
  size_t end_of_left , start_of_right;
  size_t index = cmd_line.find(first_char);
  std::string temp_string;
  end_of_left = index - 1;
  if(cmd_line[index + 1] == second_char)
  {
    second_char_exists = true;
    start_of_right = index + 2;
  }
  else
  {
    second_char_exists = false;
    start_of_right = index + 1;
  }
  temp_string = cmd_line.substr(start_of_right,string::npos);
  right_substring = _trim(temp_string);
  temp_string = cmd_line.substr(0,end_of_left + 1);
  left_substring = temp_string;
}

/* Base Command Class */

Command::Command(std::vector<std::string>& cmd_vec, std::string cmd_line):
cmd_vec_m(cmd_vec), command_line(cmd_line)
{}

Command::~Command()
{
  delete &cmd_vec_m;
}

const std::string Command::getCommandLine()
{
  return command_line;
}

const std::string Command::getCommandLine() const
{
  return command_line;
}

bool Command::_isStringDigit(std::string s)
{
  bool id_digit = false;
  for(auto it = s.begin() ; it < s.end(); it++)
  {
    id_digit = true;
    if(!(isdigit(*it))){return false;}
  }
  return id_digit;
}

/* Built-In Command Class */

BuiltInCommand::BuiltInCommand(std::vector<std::string>& cmd_vec, std::string cmd_line):
Command(cmd_vec, cmd_line)
{
  std::string last_argument = cmd_vec.back();
  if(last_argument.back() == '&')
  {
    if(last_argument.size() == 1)
    {
      cmd_vec.pop_back();
    }
    else
    {
      last_argument.pop_back();
      cmd_vec.pop_back();
      cmd_vec.push_back(last_argument);
    }
  }
}

/* Pipe Command Class */

PipeCommand::PipeCommand(std::vector<std::string>& cmd_vec, std::string cmd_line):
Command(cmd_vec, cmd_line)
{
  std::string left_command_string;
  std::string right_command_string;
  is_std_err = false;
  _getSubStrings(cmd_line,left_command_string,right_command_string,'|','&',is_std_err);
  _removeBackgroundSignString(left_command_string);
  _removeBackgroundSignString(right_command_string);

  if(left_command_string.find_last_not_of(WHITESPACE) != std::string::npos)
    command_left = SmallShell::getInstance().CreateCommand(left_command_string.c_str());
  else
    command_left = nullptr;

  if(right_command_string.find_last_not_of(WHITESPACE) != std::string::npos)
    command_right = SmallShell::getInstance().CreateCommand(right_command_string.c_str());
  else
    command_right = nullptr;
}

PipeCommand::~PipeCommand()
{
  if(command_left != nullptr)
    delete command_left;
  if(command_right != nullptr)
   delete command_right;
}

void PipeCommand::execute()
{
  int fd[2];
  if(pipe(fd) == -1)
  {
    perror("smash error: pipe failed");
    SmallShell::getInstance().updateForegroundCommand(this,0);
    return;
  }
  int std_channel;
  
  if(is_std_err)
  { 
    std_channel = STDERR_FILENO;
  }
  else
  {
    std_channel = STDOUT_FILENO;
  }

  SmallShell::getInstance().updatePipe(true);
  
  pid_t pid1 = fork();
  if(pid1 == 0)
  {
    setpgrp();
    dup2(fd[1], std_channel);
    close(fd[0]);
    close(fd[1]);
    if(command_left != nullptr)
      command_left->execute();
    SmallShell::getInstance().updateForegroundCommand(this,0);
    exit(0);
  }
  else if(pid1 == -1)
  { 
    perror("smash error: fork failed");
    close(fd[0]);
    close(fd[1]);
    SmallShell::getInstance().updateForegroundCommand(this,0);
    return;
  }
  //std::cout << "waiting for left" << std::endl;
  //std::cout << "left finished" << std::endl;
  pid_t pid2 = fork();
  if(pid2 == 0)
  {
    setpgrp();
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    close(fd[1]);
    if(command_right != nullptr)
      command_right->execute();
    SmallShell::getInstance().updateForegroundCommand(this,0);
   exit(0);
  }
  else if(pid2 == -1)
  { 
    perror("smash error: fork failed");
    close(fd[0]);
    close(fd[1]);
    SmallShell::getInstance().updateForegroundCommand(this,0);
    return;
  }
  close(fd[0]);
  close(fd[1]);
  
  waitpid(pid1,NULL,WUNTRACED);
  waitpid(pid2,NULL,WUNTRACED);
  SmallShell::getInstance().updatePipe(false);

  SmallShell::getInstance().updateForegroundCommand(this,0);
}

/* Redirection Command Class */

RedirectionCommand::RedirectionCommand(std::vector<std::string>& cmd_vec, std::string cmd_line):
Command(cmd_vec, cmd_line)
{
  std::string redirected_command_string;
  file_name = std::string();
  is_append = false;
  _getSubStrings(cmd_line,redirected_command_string,file_name,'>','>',is_append);
  _removeBackgroundSignString(redirected_command_string);
  if(redirected_command_string.find_last_not_of(WHITESPACE) != std::string::npos)
    redirected_command = SmallShell::getInstance().CreateCommand(redirected_command_string.c_str());
  else
    redirected_command = nullptr;
}

RedirectionCommand::~RedirectionCommand()
{
  if(redirected_command != nullptr)
    delete redirected_command;
}

void RedirectionCommand::execute() 
{
  int stdout_backup  = dup(STDOUT_FILENO);
  int fd = 0;
  close(STDOUT_FILENO);
  if(is_append)
  {
    fd = open(file_name.c_str(),O_WRONLY | O_CREAT | O_APPEND , S_IRUSR | S_IWUSR);
  }
  else
  {
    //remove(file_name.c_str());
    fd = open(file_name.c_str(),O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  }
  if(fd==-1)
  {
    perror("smash error: open failed");
    dup(stdout_backup);
    close(stdout_backup);
    SmallShell::getInstance().updateForegroundCommand(this,0);
    return;
  }
  if(redirected_command != nullptr)
    redirected_command->execute();
  close(fd);
  dup(stdout_backup);
  close(stdout_backup);
  SmallShell::getInstance().updateForegroundCommand(this,0);
}

/* Change Prompt Command Class*/

ChangePromptCommand::ChangePromptCommand(std::vector<std::string>& cmd_vec,std::string cmd_line):
BuiltInCommand(cmd_vec, cmd_line), prompt("smash")
{}

std::string ChangePromptCommand::getPrompt()
{
  return prompt;
}

void ChangePromptCommand::execute() 
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
	if(cmd_vec_m.size() > 1) 
  {
    prompt = cmd_vec_m[1];
  }
  SmallShell::getInstance().setPrompt(prompt);
}

/*Change Directory Class*/

ChangeDirCommand::ChangeDirCommand(std::vector<std::string>& cmd_vec, std::string cmd_line, std::string plastPwd):
BuiltInCommand(cmd_vec, cmd_line), last_pwd(plastPwd)
{}

void ChangeDirCommand::checkStatus(int status, char* curr_path)
{
  if(status == -1)
  {
    perror("smash error: chdir failed");
    return;
  }
  std::string str_path(curr_path);
  SmallShell::getInstance().updatePrevPath(str_path);
}

void ChangeDirCommand::goBack(char* curr_path)
{
  int status = 1;
  if(last_pwd.empty())
  {
    std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
    return;
  }
  const char* path = last_pwd.c_str();
  status = chdir(path);
  checkStatus(status,curr_path);
}

void ChangeDirCommand::goTo(const char* path,char* curr_path)
{
  int status = 1;
  status = chdir(path);
  checkStatus(status,curr_path);
}

void ChangeDirCommand::execute()
{
  char * curr_path = getcwd(NULL, 0);
  if(curr_path == NULL)
  {
    std::cerr << "error: getcwd failed" << std::endl;
    return;
  }

  SmallShell::getInstance().updateForegroundCommand(this,0);
	if(cmd_vec_m.size() > 2)
	{
    std::cerr << "smash error: cd: too many arguments" << std::endl;
	}
	else if(cmd_vec_m.size() > 1)
	{
		if(cmd_vec_m[1] == "-")
		{ 
      goBack(curr_path);
		}
		else{
      const char* path = cmd_vec_m[1].c_str();
		  goTo(path,curr_path);
		}
	}
	else
	{
		std::cerr << "smash error: cd: too few arguments" << std::endl;
	}
  free(curr_path);
}

/*Get Current Directory Command*/

GetCurrDirCommand::GetCurrDirCommand(std::vector<std::string>& cmd_vec, std::string cmd_line):
BuiltInCommand(cmd_vec, cmd_line)
{}

void GetCurrDirCommand::execute()
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
  char * buff = getcwd(NULL, 0);
  if(buff == NULL){std::cerr << "error: getcwd failed" << std::endl;}
	std::cout << buff << std::endl;
  free(buff);
}

/*Show PID command*/

ShowPidCommand::ShowPidCommand(std::vector<std::string>& cmd_vec, std::string cmd_line):
BuiltInCommand(cmd_vec, cmd_line)
{}

void ShowPidCommand::execute()
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
	std::cout <<"smash pid is " << SmallShell::getInstance().getSmashPid() << std::endl;
}

/*External Command Class*/

ExternalCommand::ExternalCommand(std::vector<std::string>& cmd_vec, std::string cmd_line, JobsList& jobs) :
Command(cmd_vec,cmd_line), jobs_list(jobs)
{}

void ExternalCommand::execute()
{
  std::vector<char*> argv_vec;
  std::string temp_command = command_line;
  initArgv(argv_vec,temp_command);
  char** argv = argv_vec.data();
  bool isBackground = _isBackgroundComamnd(command_line.c_str());
  if(SmallShell::getInstance().checkPipe())
  {
    execv(argv[0], &argv[0]);
    perror("smash error: exec failed");
    return;
  }
  pid_t pid = fork();
  if(pid == 0)
  {
    setpgrp();
    execv(argv[0], &argv[0]);
    perror("smash error: exec failed");
  }
  else if(pid == -1)
  {
    perror("smash error: fork failed");  
  }
  else
  {
    if(!isBackground)
    {
      SmallShell::getInstance().updateForegroundCommand(this,pid);
      int status;
      waitpid(pid, &status, WUNTRACED);
      if(WIFSTOPPED(status))
      {
        SmallShell::getInstance().updateForegroundCommand(nullptr,0);
      }
    }
    else
    {
      jobs_list.addJob(this, pid);
    }
  }
}

void ExternalCommand::initArgv(std::vector<char*>& argv_vector, std::string& command_line)
{
  argv_vector.push_back(const_cast<char*>("/bin/bash"));
  argv_vector.push_back(const_cast<char*>("-c"));
 _removeBackgroundSignString(command_line);
	argv_vector.push_back(const_cast<char*>(command_line.c_str()));
  argv_vector.push_back(NULL);
}

/*Quit Command Class*/

QuitCommand::QuitCommand(std::vector<std::string>& cmd_vec, std::string cmd_line, JobsList& jobs):
BuiltInCommand(cmd_vec, cmd_line), jobs_list(jobs)
{}

void QuitCommand::execute()
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
  if(cmd_vec_m.size() > 1 && cmd_vec_m[1] == "kill")
  {
    jobs_list.killAllJobs();
  }
  exit(0);
}

/*Jobs Command Class*/

JobsCommand::JobsCommand(std::vector<std::string>& cmd_vec,std::string cmd_line, JobsList& jobs):
BuiltInCommand(cmd_vec, cmd_line), jobs_list(jobs)
{}

void JobsCommand::execute()
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
  jobs_list.printJobsList();
}

/* Kill Command Class */

KillCommand::KillCommand(std::vector<std::string>& cmd_vec,std::string cmd_line, JobsList& jobs):
BuiltInCommand(cmd_vec, cmd_line),  jobs_list(jobs), signum(0), job_id(0)
{}

void KillCommand::execute()
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
  if(cmd_vec_m.size() != 3 || !isSignalValid() || !isJobIdValid())
  {
    std::cerr << "smash error: kill: invalid arguments" <<std::endl;
    return;
  }
  std::stringstream id_string(cmd_vec_m[2]);
  int job_id = 0;
  id_string >> job_id;
  bool exists = true;
  auto job_iter = jobs_list.getJobById(job_id,exists);
  if(!exists)
  {
    std::cerr << "smash error: kill: job-id "<< job_id <<" does not exist" <<std::endl;
    return;
  }
  std::string signal_string = cmd_vec_m[1].substr(1,string::npos);
  std::stringstream signal_stream(signal_string);
  int signal = 0;
  pid_t job_pid = job_iter->second.getJobPid();
  signal_stream >> signal;
  if(kill(job_pid,signal) == -1)
  {
    perror("smash error: kill failed");
    return;
  }
  std::cout << "signal number " << signal <<  " was sent to pid " << job_pid << std::endl;
}

bool KillCommand::isJobIdValid()
{
  std::string job_id_string = cmd_vec_m[2];
  if(job_id_string[0] == '-')
  {
    
    return _isStringDigit(job_id_string.substr(1,string::npos));
  }
  else
  {
    return _isStringDigit(job_id_string);
  }
}

bool KillCommand::isSignalValid()
{
  std::string signal_string = cmd_vec_m[1];
  if(signal_string[0] != '-'){return false;}
  if(_isStringDigit(signal_string.substr(1,string::npos)))
  {
    return true;
  }
  return false;
}
/* Foreground Command Class */
ForegroundCommand::ForegroundCommand(std::vector<std::string>& cmd_vec,std::string cmd_line, JobsList& jobs):
BuiltInCommand(cmd_vec, cmd_line), jobs_list(jobs)
{}

void ForegroundCommand::execute()
{
  JobsList::JobEntry* job;
  bool job_exists = false;
  if(cmd_vec_m.size() == 1)
  {
    if(jobs_list.isEmpty())
    {
      std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      SmallShell::getInstance().updateForegroundCommand(this,0);
      return;
    }
    job = jobs_list.getLastJob(nullptr);
  }
  else if(cmd_vec_m.size() == 2)
  {
    std::stringstream id_string(cmd_vec_m[1]);
    int job_id = 0;
    id_string >> job_id;
    auto job_iter = jobs_list.getJobById(job_id, job_exists);
    if(!job_exists || !_isStringDigit(cmd_vec_m[1]))
    {
      std::cerr << "smash error: fg: job-id "<< job_id <<" does not exist" << std::endl;
      SmallShell::getInstance().updateForegroundCommand(this,0);
      return;
    }
    job = &(job_iter->second);
  }
  else
  {
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }
  
  int job_id = job->getJobid();
  pid_t job_pid = job->getJobPid();  
  const Command* command = job->getCommand();
  SmallShell::getInstance().updateForegroundCommand(const_cast<Command*>(command),job_pid);
  jobs_list.removeJobById(job->getJobid());
  std::cout << command->getCommandLine() << " : " << job_pid << std::endl;
  kill(job_pid, SIGCONT);
  int status;
  waitpid(job_pid, &status, WUNTRACED);
  if(WIFEXITED(status))
  {
    delete command;
  }  
  else if(WIFSTOPPED(status))
  {
    jobs_list.recoverOldJobId(job_pid,job_id);
  }
  SmallShell::getInstance().updateForegroundCommand(this,0);
}

/*Background Command Class*/

BackgroundCommand::BackgroundCommand(std::vector<std::string>& cmd_vec, std::string cmd_line,JobsList& jobs):
BuiltInCommand(cmd_vec, cmd_line), jobs_list(jobs)
{}

void BackgroundCommand::execute()
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
  JobsList::JobEntry* job; 
  int job_id = 0;
  if(cmd_vec_m.size() == 1)
  {
    if(jobs_list.isStoppedListEmpty())
    {
      std::cerr << "smash error: bg: there is no stopped jobs to resume" << std::endl;
      return;    
    }
    job = jobs_list.getLastStoppedJob(&job_id);
  }
  else if(cmd_vec_m.size() == 2)
  {
    std::stringstream id_string(cmd_vec_m[1]);
    id_string >> job_id;
    bool exists = true;
    auto job_iter = jobs_list.getJobById(job_id,exists);
    if(!exists || !_isStringDigit(cmd_vec_m[1]))
    {
      std::cerr << "smash error: bg: job-id "<< job_id <<" does not exist" << std::endl;
      return;
    }
    job = &(job_iter->second);
    if(!(job->isStopped()))
    {
      std::cerr << "smash error: bg: job-id "<< job_id <<" is already running in the background" << std::endl;
      return;
    }
  }
  else
  {
    std::cerr << "smash error: bg: invalid arguments" << std::endl;
    return;
  }
  pid_t job_pid = job->getJobPid();
  const Command* command = job->getCommand();
  std::cout << command->getCommandLine() << " : " << job_pid << std::endl;
  kill(job_pid, SIGCONT);
  jobs_list.resumeJob(job_id);
}

/*Head Command Class*/

HeadCommand::HeadCommand(std::vector<std::string>& cmd_vec,std::string cmd_line):
BuiltInCommand(cmd_vec, cmd_line)
{
  num_of_lines = 10;
  if(cmd_vec.size() < 2)
  {
    std::cerr << "smash error: head: not enough arguments" << std::endl;
    return;
  }
  bool n_exists = getN(cmd_vec, num_of_lines);
  file_name = std::string();
  getFileName(cmd_vec, file_name, n_exists);
}

void HeadCommand::execute()
{
  SmallShell::getInstance().updateForegroundCommand(this,0);
  if(cmd_vec_m.size() < 2)
  {
    return;
  }
  if(file_name == "")
  {
    std::cerr << "smash error: head: not enough arguments" << std::endl;
    return;
  }
  int read_status = 0, write_status = 0;
  int read_file_fd = open(file_name.c_str(), O_RDONLY);
  if(read_file_fd == -1)
  {
    perror("smash error: open failed");
    return;
  }
  std::string curr_line;
  int counter = num_of_lines;
  while(counter != 0 && read_status != EOF_REACHED)
  {
    read_status = readLine(read_file_fd, curr_line);
    if(read_status == FAIL)
    {
      perror("smash error: read failed");
      close(read_file_fd);
      return;
    }
    write_status = writeLine(STDOUT_FILENO,curr_line);
    if(write_status == FAIL)
    {
      perror("smash error: write failed");
      close(read_file_fd);
      return;
    }
    counter --;
  }
  return;
}

int HeadCommand::readLine(int fd, std::string& line)
{
  char buffer[1];
  ssize_t result;
  while(true)
  {
    result = read(fd,buffer,1);
    if(result == -1)
    {
      return FAIL;
    }
    else if (result == 0)
    {
      return EOF_REACHED;
    }
    line.push_back(buffer[0]);
    if(buffer[0] == '\n')
    {
      return SUCCESS;
    }
  }
}

int HeadCommand::writeLine(int fd, std::string& line)
{
  ssize_t result = write(fd, line.c_str(), line.size());
  line = "";
  return result;
} 

bool HeadCommand::getN(std::vector<std::string>& cmd_vec,int& n)
{
  std::string num_string = cmd_vec[1];
  if(num_string[0] != '-')
  {
    return false;
  }
  num_string = num_string.substr(1, string::npos);
  if(!_isStringDigit(num_string))
  {
    return false;
  }
  std::stringstream num_string_stream(num_string);
  
  num_string_stream >> n;
  return true;
}

void HeadCommand::getFileName(std::vector<std::string>& cmd_vec,string& file_name, bool is_n_specified)
{
  if(is_n_specified)
  {
    if(cmd_vec.size() < 3)
    {
      file_name = "";
    }
    else{
      file_name = cmd_vec[2];
    }
  }
  else
  {
    file_name = cmd_vec[1];
  }
}

/* SmallShell Class */

SmallShell::SmallShell()
{
  foreground_pid = 0;
  foreground_command = nullptr;
  jobs = new JobsList();
  prompt = "smash";
  prev_path = std::string();
  smash_pid = getpid();
  is_pipe = false;
}

SmallShell::~SmallShell() 
{
if(foreground_command != nullptr)
{
  delete foreground_command;
}
delete jobs;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) 
{
  return CommandFactory::getCommand(cmd_line, *this);
}

void SmallShell::executeCommand(const char *cmd_line) 
{
  if(std::string(cmd_line).find_last_not_of(WHITESPACE) == std::string::npos){return;}
  Command* cmd = CreateCommand(cmd_line);
  jobs->removeFinishedJobs();
  cmd->execute();
  if(foreground_command != nullptr)
  {
    delete foreground_command;
    updateForegroundCommand(nullptr, 0);
  }
}

void SmallShell::printPrompt()
{
  std::cout << prompt << "> "; 
}

void SmallShell::setPrompt(std::string new_prompt)
{
  prompt = new_prompt;
}

void SmallShell::updateForegroundCommand(Command* new_command,pid_t new_pid)
{
  foreground_command = new_command;
  foreground_pid = new_pid;
}

void SmallShell::updatePrevPath(std::string& new_prev_path)
{
  prev_path = new_prev_path;
}

pid_t SmallShell::sendSignalToForgroundCommand(int signal)
{
  if(foreground_pid != 0)
  {   
    kill(foreground_pid,signal);
    if(signal == SIGSTOP)
    {
      jobs->addJob(foreground_command, foreground_pid, true);
    }
  }
  return foreground_pid;
}

/* Command Factory Class */

void CommandFactory::checkRedirectedOrPipe(const char* cmd_line, bool&  redirected, bool& pipe)
{
  std::string cmd_string(cmd_line);
  if(cmd_string.find('|') != string::npos)
  {
    pipe = true;
    return;
  }
  if(cmd_string.find('>') != string::npos)
  {
    redirected = true;
    return;
  }
}

Command* CommandFactory::getCommand(const char* cmd_line, SmallShell& shell)
{
  std::vector<std::string>* command_vec = _createCommandVector(cmd_line);
  bool is_pipe = false, is_redirected = false;
  checkRedirectedOrPipe(cmd_line, is_redirected, is_pipe);
  std::string command_name_clean = (*command_vec)[0];
  _removeBackgroundSignString(command_name_clean);
  if(is_pipe){return new PipeCommand(*command_vec, string(cmd_line));}
  else if(is_redirected){return new RedirectionCommand(*command_vec, string(cmd_line));}
  else if(command_name_clean == "chprompt"){return new ChangePromptCommand(*command_vec, string(cmd_line));}
  else if(command_name_clean == "showpid"){return new ShowPidCommand(*command_vec, string(cmd_line));}
  else if(command_name_clean == "pwd"){return new GetCurrDirCommand(*command_vec, string(cmd_line));}
  else if(command_name_clean == "cd"){return new ChangeDirCommand(*command_vec, string(cmd_line), shell.prev_path);}
  else if(command_name_clean == "jobs"){return new JobsCommand(*command_vec, string(cmd_line),*(shell.jobs));}
  else if(command_name_clean == "kill"){return new KillCommand(*command_vec, string(cmd_line),*(shell.jobs));}
  else if(command_name_clean == "fg"){return new ForegroundCommand(*command_vec, string(cmd_line),*(shell.jobs));}
  else if(command_name_clean == "bg"){return new BackgroundCommand(*command_vec, string(cmd_line), *(shell.jobs));}
  else if(command_name_clean == "quit"){return new QuitCommand(*command_vec, string(cmd_line), *(shell.jobs));}
  else if(command_name_clean == "head"){return new HeadCommand(*command_vec, string(cmd_line));}
  else{return new ExternalCommand(*command_vec, string(cmd_line),*(shell.jobs));}


}

/* Jobs List Class */

JobsList::JobEntry::JobEntry(Command* cmd, int id, pid_t pid, bool stop_flag):
command(cmd), job_id(id), job_pid(pid), stopped(stop_flag)
{
  time(&insert_time);
}

JobsList::JobsList():
max_id(1), running_counter(0), running_jobs_list(), stopped_jobs_list()
{}

JobsList::~JobsList()
{
  std::map<int,JobEntry>::const_iterator it1 = running_jobs_list.begin();
  std::map<int,JobEntry>::const_iterator it2 = stopped_jobs_list.begin();
  std::map<int,JobEntry>::const_iterator end1 =running_jobs_list.end();
  std::map<int,JobEntry>::const_iterator end2 = stopped_jobs_list.end();
  while((it1 != end1) && (it2 != end2))
  {
    delete it1->second.getCommand();
    delete it2->second.getCommand();
    it1++;
    it2++;
  }   
  //if one list has more elements than the other
  while(it1 != end1)
  {
    delete it1->second.getCommand();
    it1++;
  }
  while(it2 != end2)
  {
    delete it2->second.getCommand();
    it2++;
  }
}

void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped)
{
  removeFinishedJobs();
  if(running_counter == max_processes)
  {
    isStopped = true;
  }
  JobEntry job(cmd,max_id+1,pid, isStopped);
  std::pair<int, JobsList::JobEntry> new_job(max_id + 1,job);
  if(isStopped)
  {
    stopped_jobs_list.insert(new_job);
  }
  else
  {
    running_jobs_list.insert(new_job);
    running_counter++;
  }    
  updateMaxID();
}

void JobsList::printJobsList() 
{
  removeFinishedJobs();

  std::map<int,JobEntry>::const_iterator it1 = running_jobs_list.begin();
  std::map<int,JobEntry>::const_iterator it2 = stopped_jobs_list.begin();
  std::map<int,JobEntry>::const_iterator end1 =running_jobs_list.end();
  std::map<int,JobEntry>::const_iterator end2 = stopped_jobs_list.end();
  while((it1 != end1) && (it2 != end2))
  {
    if(it1->first > it2->first)
    {
      (it2->second).printJobEntry();
      it2++;
    }
    else // it2->first(job_id2) > it1->first(job_id1)
    {
      (it1->second).printJobEntry();
      it1++;
    }
  }
  
  //if one list has more elements

  while(it1 != end1)
  {
    (it1->second).printJobEntry();
    it1++;
  }
  while(it2 != end2)
  {
    (it2->second).printJobEntry();
    it2++;
  }
}

void JobsList::killAllJobs()
{
  removeFinishedJobs();
  std::map<int,JobEntry>::const_iterator it1 = running_jobs_list.begin();
  std::map<int,JobEntry>::const_iterator it2 = stopped_jobs_list.begin();
  std::map<int,JobEntry>::const_iterator end1 =running_jobs_list.end();
  std::map<int,JobEntry>::const_iterator end2 = stopped_jobs_list.end();
  pid_t job_pid;
  int jobs_num = running_jobs_list.size() + stopped_jobs_list.size();
  std::cout << "smash: sending SIGKILL signal to " << jobs_num << " jobs:" << std::endl;
  map<pid_t, std::string> killed_jobs;
  std::pair<pid_t, std::string> job_pair;
  std::string command_line;

  for(; it1!=end1 ; it1++)
  {
    job_pid = it1->second.getJobPid();
    const Command* command = (it1->second).getCommand();
    command_line = command->getCommandLine();
    job_pair = std::pair<pid_t, std::string>(job_pid, command_line);
    killed_jobs.insert(job_pair);     
  }

  for(; it2!=end2 ; it2++)
  {
    job_pid = it2->second.getJobPid();
    const Command* command = (it2->second).getCommand();
    command_line = command->getCommandLine();
    job_pair = std::pair<pid_t, std::string>(job_pid, command_line);
    killed_jobs.insert(job_pair);
  }

  for(auto it = killed_jobs.begin(); it != killed_jobs.end(); it++)
  {
    if(waitpid(it->first, NULL, WNOHANG) <= 0){
      kill(it->first, SIGKILL);
      std::cout << it->first << ": " << it->second << std::endl;
    }
  }
  return;
}

void JobsList::removeFinishedJobs()
{
  pid_t pid = waitpid(-1, NULL,WNOHANG);
  std::map<int,JobEntry>::iterator job;
  bool exists = true;
  while(pid > 0)
  {
    job = getJobByPid(pid,exists);
    if(exists)
    { 
      delete (job->second).getCommand();
      if(job->second.isStopped())
      {
        stopped_jobs_list.erase(job);
      }
      else
      {
        running_jobs_list.erase(job);
        running_counter--;
      }       
    }
    pid = waitpid(-1, NULL,WNOHANG);
  }
  updateMaxID();
}

void JobsList::updateMaxID()
{
  int a = 0, b = 0;
  getLastRunningJob(&a);
  getLastStoppedJob(&b);
  if(a > b){max_id = a;}
  else{max_id = b;}
}

std::map<int,JobsList::JobEntry>::iterator JobsList::getJobById(int jobId, bool& exists)
{
  std::map<int,JobEntry>::iterator it1 = running_jobs_list.begin();
  std::map<int,JobEntry>::iterator it2 = stopped_jobs_list.begin();
  std::map<int,JobEntry>::iterator end1 =running_jobs_list.end();
  std::map<int,JobEntry>::iterator end2 = stopped_jobs_list.end();
  exists = true;

  while((it1 != end1) && (it2 != end2))
  {
    if(it1->first == jobId)
    {
      return it1;
    }
    else if (it2->first == jobId)
    {
      return it2;
    }
    it1++;
    it2++;
  }   

  //if one list has more elements
  while(it1 != end1)
  {
    if(it1->first == jobId)
    {
      return it1;
    }
    it1++;
  }

  while(it2 != end2)
  {
    if (it2->first == jobId)
    {
      return it2;
    }      
    it2++;
  }
  exists = false;
  return it1;
}

std::map<int,JobsList::JobEntry>::iterator JobsList::getJobByPid(int jobPid, bool& exists)
{
  std::map<int,JobEntry>::iterator it1 = running_jobs_list.begin();
  std::map<int,JobEntry>::iterator it2 = stopped_jobs_list.begin();
  std::map<int,JobEntry>::iterator end1 =running_jobs_list.end();
  std::map<int,JobEntry>::iterator end2 = stopped_jobs_list.end();
  exists = true;

  while((it1 != end1) && (it2 != end2))
  {
    if(it1->second.getJobPid() == jobPid)
    {
      return it1;
    }
    else if (it2->second.getJobPid() == jobPid)
    {
      return it2;
    }
    it1++;
    it2++;
  }   

  //if one list has more elements
  while(it1 != end1)
  {
    if(it1->second.getJobPid() == jobPid)
    {
      return it1;
    }
    it1++;
  }

  while(it2 != end2)
  {
    if (it2->second.getJobPid() == jobPid)
    {
      return it2;
    }      
    it2++;
  }

  exists = false;
  return it1;
}


void JobsList::removeJobById(int jobId)
{
    bool exists = true;
    std::map<int,JobEntry>::iterator job = getJobById(jobId,exists);
    if(!exists)
    {
      return;
    }
    if((job->second).isStopped() == true)
    {
      stopped_jobs_list.erase(job);
    }
    else
    {
      running_jobs_list.erase(job);
    }
    return;
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJobId)
{
  int a = 0 , b = 0;
  JobEntry* last_running = getLastRunningJob(&a);
  JobEntry* last_stopped = getLastStoppedJob(&b);
  if(a==0 && b==0){return nullptr;}
  if(a>b)
  { 
    if(lastJobId != NULL){*lastJobId = a;}
    return last_running;
  }
  else{
        if(lastJobId != NULL){*lastJobId = b;}
        return last_stopped;
      }
}

JobsList::JobEntry * JobsList::getLastRunningJob(int* lastJobId)
{
  std::map<int,JobEntry>::reverse_iterator it = running_jobs_list.rbegin();
  if(it == running_jobs_list.rend()){return nullptr;}
  if(lastJobId != nullptr)
  {
    *lastJobId = it->first;
  }
  return &(it->second);
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId)
{
  std::map<int,JobEntry>::reverse_iterator it = stopped_jobs_list.rbegin();
  if(it == stopped_jobs_list.rend()){return nullptr;}
  if(jobId != nullptr)
  {
    *jobId = it->first;
  }
  return &(it->second);
}

void JobsList::JobEntry::stopJob()
{
  stopped = true;
}
void JobsList::JobEntry::resumeJob()
{
  stopped = false;
}

void JobsList::JobEntry::printJobEntry() const
{
  std::string command_line = command->getCommandLine();
  time_t current_time;
  time(&current_time);
  auto diff = difftime(current_time, insert_time);
  std::cout << "[" << job_id << "] " << command_line << " : " << job_pid << " " << int(diff) << " secs ";
  if(stopped)
  {
    std::cout << "(stopped)";
  }
  std::cout << std::endl;
}
   
void JobsList::resumeJob(int job_id)
{
  bool exists = true;
  auto job_iter = getJobById(job_id,exists);
  if(!exists)
  {
    return;
  }
  JobEntry* job = &(job_iter->second);
  if(!job->isStopped())
  {
    return;
  }
  else
  {
    JobEntry job_copy(*job);
    job_copy.resumeJob();
    running_jobs_list.insert(std::pair<int,JobEntry>(job_id, job_copy));
    stopped_jobs_list.erase(job_iter);
  }
}

void JobsList::recoverOldJobId(pid_t job_pid,int job_id)
{
  bool exist = true;
  auto job = getJobByPid(job_pid,exist);
  JobEntry new_entry(job->second);
  new_entry.changeJobId(job_id);
  stopped_jobs_list.erase(job);
  stopped_jobs_list.insert(std::pair<int,JobEntry>(job_id, new_entry));
}
