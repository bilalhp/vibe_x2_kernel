/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>
   Copyright (C) 2010 Konstantin Belousov <kib@freebsd.org>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include "_UPT_internal.h"
#ifdef HAVE_AEE_FEATURE
#include "aee.h"
extern uintptr_t FirstSP;
extern struct aee_thread_user_stack UserStackInfo;
extern int IsKernelDumpUserStack;
#endif

#if HAVE_DECL_PTRACE_POKEDATA || HAVE_TTRACE
int
_UPT_access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val,
		 int write, void *arg)
{
  struct UPT_info *ui = arg;
  if (!ui)
	return -UNW_EINVAL;

  pid_t pid = ui->pid;

  errno = 0;
  if (write)
    {
      Debug (16, "mem[%lx] <- %lx\n", (long) addr, (long) *val);
#ifdef HAVE_TTRACE
#	warning No support for ttrace() yet.
#else
      /* ANDROID support update. */
      ptrace (PTRACE_POKEDATA, pid, (void*) (uintptr_t) addr, (void*) (uintptr_t) *val);
      /* End of ANDROID update. */
      if (errno)
	return -UNW_EINVAL;
#endif
    }
  else
    {
#ifdef HAVE_TTRACE
#	warning No support for ttrace() yet.
/* ANDROID support update. */
#elif defined(__mips__) && _MIPS_SIM == _ABIO32
      /* The assumption is that sizeof(long) == sizeof(unw_word_t).
         This isn't true for this mips abi, so it requires two reads to get
         the entire 64 bit value. */
      long reg1, reg2;
      reg1 = ptrace (PTRACE_PEEKDATA, pid, (void*) (uintptr_t) addr, 0);
      if (errno)
	return -UNW_EINVAL;
      reg2 = ptrace (PTRACE_PEEKDATA, pid, (void*) (uintptr_t) (addr + sizeof(long)), 0);
      if (errno)
	return -UNW_EINVAL;
      *val = ((unw_word_t)(reg2) << 32) | (uint32_t) reg1;
#else
      /* ANDROID support update. */
      *val = ptrace (PTRACE_PEEKDATA, pid, (void*) addr, 0);
      /* End of ANDROID update. */
      if (errno)
#ifdef HAVE_AEE_FEATURE
      {
        bool ret = false;
      if( (IsKernelDumpUserStack)&& (addr> FirstSP)&& (addr < (FirstSP + UserStackInfo.stacklength)) )
      {
          #if __LP64__
              //64bit user ok, 32bit user????
              memcpy(val,&(UserStackInfo.Userspace_Stack[addr-FirstSP]),8);
          #else
              memcpy(val,&(UserStackInfo.Userspace_Stack[addr-FirstSP]),4);
          #endif
          //*val=UserStackInfo.Userspace_Stack[(addr-FirstSP)/4 ];
          Debug (16, "stack: mem[%lx] -> %lx\n", (long) addr, (long) *val);
          ret=true;
      
      }
      else
        	ret = aee_try_get_word(pid, addr, val);
        if (!ret)
#endif //HAVE_AEE_FEATURE
	return -UNW_EINVAL;
#ifdef HAVE_AEE_FEATURE
      }
#endif //HAVE_AEE_FEATURE
#endif
/* End of ANDROID update. */
      Debug (16, "mem[%lx] -> %lx\n", (long) addr, (long) *val);
    }
  return 0;
}
#elif HAVE_DECL_PT_IO
int
_UPT_access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val,
		 int write, void *arg)
{
  struct UPT_info *ui = arg;
  if (!ui)
	return -UNW_EINVAL;
  pid_t pid = ui->pid;
  struct ptrace_io_desc iod;

  iod.piod_offs = (void *)addr;
  iod.piod_addr = val;
  iod.piod_len = sizeof(*val);
  iod.piod_op = write ? PIOD_WRITE_D : PIOD_READ_D;
  if (write)
    Debug (16, "mem[%lx] <- %lx\n", (long) addr, (long) *val);
  if (ptrace(PT_IO, pid, (caddr_t)&iod, 0) == -1)
    return -UNW_EINVAL;
  if (!write)
     Debug (16, "mem[%lx] -> %lx\n", (long) addr, (long) *val);
  return 0;
}
#else
#error Fix me
#endif
