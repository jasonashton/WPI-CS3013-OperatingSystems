#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
// For copy_to_user() and copy_from_user()
#include <asm/uaccess.h>
// For kmalloc(), kfree()
#include <linux/slab.h>
// For for_each_process(), task_struct
#include <linux/sched.h>
// For current
#include <asm-generic/current.h>

unsigned long **sys_call_table;

asmlinkage long (*ref_sys_cs3013_syscall1)(void);
asmlinkage long (*ref_sys_cs3013_syscall2)(void);
asmlinkage long (*ref_sys_cs3013_syscall3)(void);

asmlinkage long new_sys_cs3013_syscall1(void) {
  printk(KERN_INFO "\"'Hello world?!' More like 'Goodbye, world!' EXTERMINATE!\" -- Dalek");
  return 0;
}

int search_syscall2(unsigned short *target_pid, unsigned short *target_uid, struct task_struct *parent_to_browse,
                    int(*tocall)(unsigned short *, struct task_struct *)) {
  struct task_struct *child; // Hold the children of the parent
  // Browse through the children first, cover the tree below the current now
  list_for_each_entry(child, &(parent_to_browse->tasks), tasks) {
    // Check the current process
    if(child->pid == *target_pid) {
      // We found the matching process
      return (*tocall)(target_uid, child);
    }
    // Let the code continue as we still haven't found the required process
  }
  // If no matches, return ESRCH
  printk(KERN_INFO "User ID %d tried to change loginuid of process with PID %d but the process was not found\n", current_uid().val, *target_pid);
  return ESRCH; // No such process found
}

int invoke_syscall2(unsigned short *target_uid, struct task_struct *task) {
  kuid_t target_uid_kuid; // Kernel UID struct with the requested UID
  target_uid_kuid.val = *target_uid;
  // Now process the UID information
  if(current_uid().val >= 1000) {
    // Non-privilieged user. Check before proceeding
    if( ( ( (task->cred->uid.val == current_uid().val ||                   // Check the cred structure if the user UID matches; or
             task->real_cred->uid.val == current_uid().val) &&             // if the real_cread structure has a matching user UID;
	   ( task->loginuid.val == (-1) || task->loginuid.val == 65535 ) ) // if and only if the loginuid value is -1 (the default linux base value)
	                                                                   //  or 65536 (unsigned short, -1 overflow)
	   || ( task->loginuid.val == current_uid().val )		   // Else check if the loginuid matches the current uid
	 ) && (*target_uid == 1001) )
    {
      printk(KERN_INFO "Process with PID %d created by user %d is changing loginuid to %d\n", task->pid, task->cred->uid.val, *target_uid);
      task->loginuid = target_uid_kuid; // Change the UID of the process to the desired one
    } else {
      printk(KERN_INFO "User with UID %d attempted to change loginuid of process with PID %d and was not allowed as it is not authorized to do so\n", current_uid().val, task->pid);
      return EPERM; // Operation not permitted
    }
  } else {
    // We got a prviliged user. Let it do anything
    printk(KERN_INFO "Priviliged user with UID %d changed loginuid of process with PID %d to %d\n", current_uid().val, task->pid, *target_uid);
    task->loginuid = target_uid_kuid; // Change the UID of the process
  }
  // Return success
  return 0;
}

asmlinkage long new_sys_cs3013_syscall2(unsigned short *target_pid, unsigned short *target_uid) {
  // Grab the PID targetted and the UID to switch to and copy to kernel space
  // First all the declarations
  unsigned short *ktarget_pid; // Kernel space location for target_pid short
  unsigned short *ktarget_uid; // Kernel space location for target_uid short
  int returnVal = 0; // Return value of the syscall2 search
  // Now allocate memory and return errors if necessary
  ktarget_pid = (unsigned short*)kmalloc(sizeof(unsigned short), GFP_KERNEL);
  ktarget_uid = (unsigned short*)kmalloc(sizeof(unsigned short), GFP_KERNEL);
  // Attempt to make copies and check if the memory was allocated or not
  if(!ktarget_pid || copy_from_user(ktarget_pid, target_pid, sizeof(unsigned short)))
    return EFAULT;
  if(!ktarget_uid || copy_from_user(ktarget_uid, target_uid, sizeof(unsigned short)))
    return EFAULT;
  // Now do some processing and grab the required return value
  // We will scan all children and their children and so on starting at the INIT of Linux which is the
  //  super-parent of all processes. init_task refers to the task_struct of the init process
  returnVal = search_syscall2(ktarget_pid, ktarget_uid, &init_task, invoke_syscall2);
  // Free up the kernel space taken up
  kfree(ktarget_pid);
  kfree(ktarget_uid);
  // Finally, return
  return returnVal;
}

int search_syscall3(unsigned short *target_pid, unsigned short *actual_uid, struct task_struct *parent_to_browse,
                    unsigned short(*tocall)(struct task_struct *)) {
  struct task_struct *child; // Hold the children of the parent
  // Browse the children and cover the tree below the current process
  list_for_each_entry(child, &(parent_to_browse->tasks), tasks) {
    // Process the process to see if the PID matches
    if(child->pid == *target_pid) {
      // We found the matching process
      unsigned short actual_proc_uid = (*tocall)(child);
      if(copy_to_user(actual_uid, &actual_proc_uid, sizeof(unsigned short)))
        return EFAULT; // Bad address
      printk(KERN_INFO "User with user ID %d queried loginuid of process with PID %d and the returned value is %d\n", current_uid().val, child->pid, actual_proc_uid);
      return 0; // Successful
    }
    // If we didn't find it continue to loop over to find the process
  }
  // If no matches, return ESRCH
  printk(KERN_INFO "User with ID %d attempted to get loginuid of process with ID %d but the process was not found\n", current_uid().val, *target_pid);
  return ESRCH; // No such process found
}

unsigned short invoke_syscall3(struct task_struct *task) {
  // Grab the required UID information
  return ( ( task->loginuid.val == (65535) || task->loginuid.val == (-1) ) ? ( (!task->cred->uid.val) ? (task->real_cred->uid.val) : (task->cred->uid.val) ) : task->loginuid.val );
  // So, check if the loginuid is -1 or 65535, not a valid UID. Just return the current UID
  // If that's not the case, return the required loginuid
}

asmlinkage long new_sys_cs3013_syscall3(unsigned short *target_pid, unsigned short *actual_uid) {
  // Let us make some data definitions now
  unsigned short *ktarget_pid; // Kernel space location for the target_pid short
  int returnValue = 0; // Return value from the syscall3 function 
  // Now allocate the necessary kernel memory
  ktarget_pid = (unsigned short*)kmalloc(sizeof(unsigned short), GFP_KERNEL);
  // Attempt to make the copy and see if the memory was allocated or not
  if(!ktarget_pid || copy_from_user(ktarget_pid, target_pid, sizeof(unsigned short)))
    return EFAULT;
  // Now prepare to loop over the processes
  // We will scan all children and their children and so on starting at the INIT of Linux which is the
  //  super-parent of all processes. init_task refers to the task_struct of the init process
  returnValue = search_syscall3(ktarget_pid, actual_uid, &init_task, invoke_syscall3);
  // Free up the kernel space taken up
  kfree(ktarget_pid);
  // Now return
  return returnValue; 
}

static unsigned long **find_sys_call_table(void) {
  unsigned long int offset = PAGE_OFFSET;
  unsigned long **sct;
  
  while (offset < ULLONG_MAX) {
    sct = (unsigned long **)offset;

    if (sct[__NR_close] == (unsigned long *) sys_close) {
      printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX",
	     (unsigned long) sct);
      return sct;
    }
    
    offset += sizeof(void *);
  }
  
  return NULL;
}

static void disable_page_protection(void) {
  /*
    Control Register 0 (cr0) governs how the CPU operates.

    Bit #16, if set, prevents the CPU from writing to memory marked as
    read only. Well, our system call table meets that description.
    But, we can simply turn off this bit in cr0 to allow us to make
    changes. We read in the current value of the register (32 or 64
    bits wide), and AND that with a value where all bits are 0 except
    the 16th bit (using a negation operation), causing the write_cr0
    value to have the 16th bit cleared (with all other bits staying
    the same. We will thus be able to write to the protected memory.

    It's good to be the kernel!
  */
  write_cr0 (read_cr0 () & (~ 0x10000));
}

static void enable_page_protection(void) {
  /*
   See the above description for cr0. Here, we use an OR to set the 
   16th bit to re-enable write protection on the CPU.
  */
  write_cr0 (read_cr0 () | 0x10000);
}

static int __init interceptor_start(void) {
  /* Find the system call table */
  if(!(sys_call_table = find_sys_call_table())) {
    /* Well, that didn't work. 
       Cancel the module loading step. */
    return -1;
  }
  
  /* Store a copy of all the existing functions */
  ref_sys_cs3013_syscall1 = (void *)sys_call_table[__NR_cs3013_syscall1];
  ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];
  ref_sys_cs3013_syscall3 = (void *)sys_call_table[__NR_cs3013_syscall3];

  /* Replace the existing system calls */
  disable_page_protection();

  sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)new_sys_cs3013_syscall1;
  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)new_sys_cs3013_syscall2;
  sys_call_table[__NR_cs3013_syscall3] = (unsigned long *)new_sys_cs3013_syscall3;
  
  enable_page_protection();
  
  /* And indicate the load was successful */
  printk(KERN_INFO "Loaded interceptor!");

  return 0;
}

static void __exit interceptor_end(void) {
  /* If we don't know what the syscall table is, don't bother. */
  if(!sys_call_table)
    return;
  
  /* Revert all system calls to what they were before we began. */
  disable_page_protection();
  sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)ref_sys_cs3013_syscall1;
  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)ref_sys_cs3013_syscall2;
  sys_call_table[__NR_cs3013_syscall3] = (unsigned long *)ref_sys_cs3013_syscall3;
  enable_page_protection();

  printk(KERN_INFO "Unloaded interceptor!");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);
