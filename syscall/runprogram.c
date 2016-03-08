/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>

/*
	Sets up our stack address space to load arguments that are aligned correctly
	-We need to fill the stack by first decrementing stack pointer enough space to align
		an argument. Then after that's done we load in the argument onto the stack
	-Repeat that process until all of our arguments are loaded
	-Go back into runprogram and use copyout to copy the args back to user space for execution
*/
void set_stack(vaddr_t stackptr)
{

}

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.

   TODO: need to add argument processing
 */
int
runprogram(char *progname, char **arguments)
{
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result, arglen;
	int index = 0;

	while(arguments[index] != NULL) {
		index++;
	}

	char **all_arguments = (char**)kmalloc(sizeof(char*)*index);
	index = 0;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	//Prepare Args parsing

	/* We should be a new process. */
	KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	//Parse Arguments here for transport to user space
	while(arguments[index] != NULL)	{
		char *arg;
		len = strlen(arguments[index]) + 1; // +1 for null terminator

		int oglen = len;
		//Find space of argument and pad it to be divisible by 4
		if(len % 4 != 0) {
			len = len + (4 - len % 4);
		}

		arg = kmalloc(sizeof(len));
		arg = kstrdup(args[index]);

		int i;
		for(i = 0; i < len; i++)
		{
			if(i <= oglen)
				arg[i] = '\0';
			else
				arg[i] = args[index][i];
		}

		//Decrement stackpointer to make space to load our arg
		stackptr -= len;

		//copyout our arg to our stack
		result = copyout((const void *)arg, (userptr_t)stackptr, (size_t) len);
		if(result){
			kprintf("Copyout failed\n");
			return result;
		}
		//deallocate the char pointer for this arg
		kfree(arg);
		all_arguments[index] = (char *)stackptr; 

		index++;
	}

	if(arguments[index] == NULL)
	{
		//handle null terminator
		stackptr -= 4 * sizeof(char);
	}

	int i;
	for(i = (index - 1); i >= 0; i--){
		stackptr = stackptr - sizeof(char*);
		result = copyout(const void *)(all_arguments + i), (userptr_t)stackptr, (sizeof(char *)));
		if(result){
			kprintf("Copyout failed, result %d, Array Index %d\n", result, i);
			return result;
		}
	}
	/* Warp to user mode. */
	enter_new_process(index /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}



