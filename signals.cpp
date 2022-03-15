#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  std::cout << "smash: got ctrl-Z" << std::endl;
  pid_t pid = (SmallShell::getInstance()).sendSignalToForgroundCommand(SIGSTOP);
  if(pid != 0) //an active foreground job got the signal
  {
    std::cout << "smash: process " << pid << " was stopped" << std::endl;
  }
}

void ctrlCHandler(int sig_num) {
  std::cout << "smash: got ctrl-C" << std::endl;
  pid_t pid = SmallShell::getInstance().sendSignalToForgroundCommand(SIGKILL);
  if(pid != 0) //an active foreground job got the signal
  {
    std::cout << "smash: process " << pid << " was killed" << std::endl;
  }
}


