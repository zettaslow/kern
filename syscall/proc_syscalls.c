#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>


void destroy_process(pid_t pid)
{
  struct process_table_entry *temp, *temp2;
  for(temp=process_table; temp->next->pid != pid; temp = temp->next);
  temp2 = temp->next;
  temp->next = temp->next->next;
  kfree(temp2->proc);
  kfree(temp2);
}
  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {
  #if OPT_A2
  /*what Needs to be done 
    -Write Entry to page table
    -Loop through process's children processes
       -If child process has not exited, quit them by
          -Going through child's children
              -Wait for them to finish
              -set exited boolean to true
              -Raise semaphore of process
       -If child process has exited
          -Destroy process of current thread's pid
    
      Then Exit thread when done

      Destroy process by removing references to it in process table
      ie: Move all other processes up by 1
      Then free memory on the ram.
  */
  struct process_table_entry *temp;
  //Go to the very last process
  for(temp = process_table; temp->pid != curthread->parent_pid || temp == NULL; temp=temp->next);

  if(temp == NULL)
  {
    //do nothing
  }
  else if(temp->proc->exited == 0)
  {
    struct process_table_entry *temp2;
    //Go to last unexited child
    for(temp2 = process_table; temp2->pid != curthread->pid || temp2 == NULL; temp2=temp2->next);
    if(temp2 == NULL)
    {
      kprintf("Process with PID %d not present in process table to exit\n", curthread->pid);
    }
    temp2->proc->exited = 1;
    V(temp2->proc->exit_semaphore);
  }
  else
  {
    destroy_process(curthread->pid);
  }

  thread_exit();
  #else
  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
  #endif
}

/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  #if OPT_A2
  *retval = curthread->t_pid;
  return 0;
  #else
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;
  return(0);
  #endif
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  #if OPT_A2
  #else
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
  #endif
}

int sys_fork(struct trapframe *tf, pid_t *return_value)
{
  #if OPT_A2
  return 1;
  #else
  #endif
}

int sys_execv(userptr_t prog, userptr_t args)
{
  #if OPT_A2
  return 1;
  #else
  #endif
}