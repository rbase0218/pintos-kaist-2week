#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#ifdef VM
#include "vm/vm.h"
#endif

static void process_cleanup(void);
static bool load(const char *file_name, struct intr_frame *if_);
static void initd(void *f_name);
static void __do_fork(void *);

/* General process initializer for initd and other process. */

// 설명 : 프로세스 초기화
// 인자 : 인자 없음
// 반환 값 : 없음

static void
process_init(void)
{
	// 구조체 타입의 스레드 포인터 타입이 가리키는 메모리 주소에 thread_current()의 리턴값 할당
	// thread_current()는 현재 실행 중인 스레드의 메모리 주소를 반환 
	struct thread *current = thread_current();
}

/* Starts the first userland program, called "initd", loaded from FILE_NAME.
 * The new thread may be scheduled (and may even exit)
 * before process_create_initd() returns. Returns the initd's
 * thread id, or TID_ERROR if the thread cannot be created.
 * Notice that THIS SHOULD BE CALLED ONCE. */
/* FILE_NAME에서 로드된 "initd"라는 첫 번째 사용자 프로그램을 시작합니다.
 * 새로 생성된 스레드는 process_create_initd()가 반환되기 전에 
 * 스케줄링될 수 있으며 (심지어 종료될 수도 있습니다).
 * 생성된 스레드의 ID를 반환하며, 스레드를 생성할 수 없는 경우에는 TID_ERROR를 반환합니다.
 * 주의: 이 함수는 반드시 **한 번만 호출**되어야 합니다. */

// 설명 : 프로세스를 생성 및 초기화
// 인자 : 문자 포인터 타입의 file_name
// 반환값 : tid (스레드ID) 
tid_t process_create_initd(const char *file_name)
{
	char *fn_copy;
	tid_t tid;

	/* Make a copy of FILE_NAME.
	 * Otherwise there's a race between the caller and load(). */
	// file_name의 복사본을 만든다.. 
	// 안그러면 호출자와 load()	사이에 경쟁 조건이 생길 수 있다.  
	fn_copy = palloc_get_page(0); 
	if (fn_copy == NULL)
		return TID_ERROR;
	strlcpy(fn_copy, file_name, PGSIZE);

	/* Create a new thread to execute FILE_NAME. */ 
	// file_name을 실행할 새로운 스레드를 생성한다. 
	tid = thread_create(file_name, PRI_DEFAULT, initd, fn_copy);
	if (tid == TID_ERROR)
		palloc_free_page(fn_copy);
	return tid;
}


/* A thread function that launches first user process. */
// 설명 : 첫 번째 사용자 프로세스를 실행하는 스레드 함수
// 인자 : 실행할 사용자 프로그램 이름을 가리키는 void 포인터 타입의 f_name
// 반환값 : 없음
static void
initd(void *f_name) // init daemon 
{
#ifdef VM
	supplemental_page_table_init(&thread_current()->spt);
#endif

	process_init(); // 프로세스 초기화 실행 

	if (process_exec(f_name) < 0) // process_exec가 음수를 반환하면 프로그램 실행 실패, 패닉
 		PANIC("Fail to launch initd\n");
	NOT_REACHED(); // -> 얜 뭐야? 
}


/* Clones the current process as `name`. Returns the new process's thread id, or
 * TID_ERROR if the thread cannot be created. */
/* 현재 프로세스를 `name`이라는 이름으로 복제합니다.
 * 새로 생성된 프로세스의 스레드 ID를 반환하며,
 * 스레드를 생성할 수 없는 경우 TID_ERROR를 반환합니다. */

// 설명 : 프로세스 복제해서 name 부여
// 인자 : 복제된 프로세스에 할당할 이름 (const char *name)
// 반환 : 새로 생성된 프로세스의 스레드 ID 
tid_t process_fork(const char *name, struct intr_frame *if_ UNUSED)
{
	/* Clone current thread to new thread.*/
	return thread_create(name,
						 PRI_DEFAULT, __do_fork, thread_current());
}



#ifndef VM
/* Duplicate the parent's address space by passing this function to the
 * pml4_for_each. This is only for the project 2. */
/* 부모의 주소 공간을 복제하기 위해 이 함수를 pml4_for_each에 전달합니다.
 * 이는 프로젝트 2에서만 사용됩니다. */

// 설명 : 페이지 테이블 엔트리를 복제
// 인자 : 페이지 테이블 엔트리 (pte), 가상 주소 (va), 부모 스레드를 가리키는 보조 데이터 (aux)
// 반환 : true
static bool
duplicate_pte(uint64_t *pte, void *va, void *aux)
{
	struct thread *current = thread_current();
	struct thread *parent = (struct thread *)aux;
	void *parent_page;
	void *newpage;
	bool writable;

	/* 1. TODO: If the parent_page is kernel page, then return immediately. */
	/* 1. TODO: 부모 페이지가 커널 페이지라면 즉시 반환합니다. */

	/* 2. Resolve VA from the parent's page map level 4. */
	/* 2. 부모의 최상위 페이지 맵 레벨 4(pml4)에서 VA를 참조합니다. */
	parent_page = pml4_get_page(parent->pml4, va);

	/* 3. TODO: Allocate new PAL_USER page for the child and set result to
	 *    TODO: NEWPAGE. */
	/* 3. TODO: 자식 프로세스를 위해 PAL_USER 페이지를 새로 할당하고
	 *    TODO: 결과를 NEWPAGE에 저장합니다. */


	/* 4. TODO: Duplicate parent's page to the new page and
	 *    TODO: check whether parent's page is writable or not (set WRITABLE
	 *    TODO: according to the result). */
	/* 4. TODO: 부모의 페이지를 새 페이지에 복사하고,
	 *    TODO: 부모 페이지가 쓰기 가능한지 확인합니다.
	 *    TODO: 결과에 따라 WRITABLE 값을 설정합니다. */

	/* 5. Add new page to child's page table at address VA with WRITABLE
	/* 5. VA 주소에 WRITABLE 권한으로 자식의 페이지 테이블에 새 페이지를 추가합니다. */
	/*    permission. */
	/* 	  권한         */
	if (!pml4_set_page(current->pml4, va, newpage, writable))
	{
		/* 6. TODO: if fail to insert page, do error handling. */
		/* 6. TODO: 페이지 삽입에 실패한 경우 오류 처리를 수행합니다. */
	}
	return true;
}
#endif

/* A thread function that copies parent's execution context. 
 * 부모의 실행 컨텍스트를 복사하는 스레드 함수입니다.
 * Hint) parent->tf does not hold the userland context of the process. 
 * 힌트) parent->tf는 프로세스의 사용자 영역 컨텍스트를 포함하지 않습니다.
 *       That is, you are required to pass second argument of process_fork to 
 *       this function.
 *       즉, process_fork의 두 번째 인자를 이 함수에 전달해야 합니다. */  

// 설명 : 부모의 실행 컨텍스트를 fork하는 함수 
// 인자 : aux가 뭐냐...? 
// 반환 : 없음 
static void
__do_fork(void *aux)
{
	struct intr_frame if_;
	struct thread *parent = (struct thread *)aux;
	struct thread *current = thread_current();
	/* TODO: somehow pass the parent_if. (i.e. process_fork()'s if_) 
	 * TODO: parent_if를 전달하는 방법을 구현하세요. (즉, process_fork()의 if_) */	
	struct intr_frame *parent_if;
	bool succ = true;

	/* 1. Read the cpu context to local stack. 
	 * 1. CPU 컨텍스트를 로컬 스택으로 읽어옵니다. */
	memcpy(&if_, parent_if, sizeof(struct intr_frame));

	/* 2. Duplicate PT 
	 * 2. 페이지 테이블(PT)을 복제합니다. */
	current->pml4 = pml4_create();
	if (current->pml4 == NULL)
		goto error;

	process_activate(current);
#ifdef VM
	supplemental_page_table_init(&current->spt);
	if (!supplemental_page_table_copy(&current->spt, &parent->spt))
		goto error;
#else
	if (!pml4_for_each(parent->pml4, duplicate_pte, parent))
		goto error;
#endif

	/* TODO: Your code goes here.
	 * TODO: 여기에 코드를 작성하세요.
	 * TODO: Hint) To duplicate the file object, use `file_duplicate`
	 * TODO:       in include/filesys/file.h. Note that parent should not return
	 * TODO:       from the fork() until this function successfully duplicates
	 * TODO:       the resources of parent.
	 * TODO: 힌트) 파일 객체를 복제하려면 include/filesys/file.h에 있는 
	 * TODO:       `file_duplicate`를 사용하세요. 부모는 이 함수가 부모의
	 * TODO:       리소스를 성공적으로 복제할 때까지 fork()에서 반환되지 않아야 합니다. */

	process_init();

	/* Finally, switch to the newly created process. 
	 * 마지막으로 새로 생성된 프로세스로 전환합니다. */
	if (succ)
		do_iret(&if_);
error:
	thread_exit();
}

/* Switch the current execution context to the f_name.
 * Returns -1 on fail. */
// 설명 : 현재 실행되고 있는 Context를 인자(f_name)로 전환한다.
// 반환 값 : 실패시 -1을 반환한다.
int process_exec(void *f_name)
{
	char *file_name = f_name;
	bool success;

	msg("⭐ %s\n", f_name);

	/* We cannot use the intr_frame in the thread structure.
	 * This is because when current thread rescheduled,
	 * it stores the execution information to the member. */
	// 스레드 구조체의 intr_frame은 사용 불가.
	// 현재 Thread가 스케줄링 될 때, 실행 정보를 저장하기 때문.
	struct intr_frame _if;
	_if.ds = _if.es = _if.ss = SEL_UDSEG;
	_if.cs = SEL_UCSEG;
	_if.eflags = FLAG_IF | FLAG_MBS;

	/* We first kill the current context */
	// 현재 실행중인 Context를 종료한다.
	process_cleanup();

	/* And then load the binary */
	// Binary를 호출한다.
	success = load(file_name, &_if);

	/* If load failed, quit. */
	// 만약 로드에 실패하면 종료한다.
	palloc_free_page(file_name);
	if (!success)
		return -1;

	/* Start switched process. */
	// 전환된 프로세스를 시작한다.
	do_iret(&_if);

	NOT_REACHED();
}

/* Waits for thread TID to die and returns its exit status.  If
 * it was terminated by the kernel (i.e. killed due to an
 * exception), returns -1.  If TID is invalid or if it was not a
 * child of the calling process, or if process_wait() has already
 * been successfully called for the given TID, returns -1
 * immediately, without waiting.
 *
 * This function will be implemented in problem 2-2.  For now, it
 * does nothing. */ 

/* 스레드 TID가 종료될 때까지 대기하고 해당 스레드의 종료 상태를 반환합니다.
 * 만약 커널에 의해 종료되었을 경우(즉, 예외로 인해 강제 종료된 경우) -1을 반환합니다.
 * TID가 유효하지 않거나, 호출 프로세스의 자식 프로세스가 아니거나,
 * 혹은 주어진 TID에 대해 process_wait()가 이미 성공적으로 호출된 경우,
 * 기다리지 않고 즉시 -1을 반환합니다.
 *
 * 이 함수는 문제 2-2에서 구현될 예정입니다. 현재는 아무 동작도 하지 않습니다. */

// 설명 : 프로세스 대기하는 함수. 왜 대기하는지는 모름
// 인자 : 자식 프로세스의 id 
// 반환 : -1 
int process_wait(tid_t child_tid UNUSED)
{
	/* XXX: Hint) The pintos exit if process_wait (initd), we recommend you
	 * XXX:       to add infinite loop here before
	 * XXX:       implementing the process_wait. */

	/* XXX: 힌트) process_wait(initd)를 호출하면 PintOS가 종료됩니다. 
 	* XXX:       process_wait를 구현하기 전에 여기에서 무한 루프를 추가할 것을 권장합니다. */

	return -1;
}


/* Exit the process. This function is called by thread_exit (). */
/* 프로세스를 종료합니다. 이 함수는 thread_exit()에 의해 호출됩니다. */
// 설명 : 프로세스 종료
// 인자 : void
// 반환 : 없음 
void process_exit(void)
{
	struct thread *curr = thread_current();
	/* TODO: Your code goes here.
	 * TODO: Implement process termination message (see
	 * TODO: project2/process_termination.html).
	 * TODO: We recommend you to implement process resource cleanup here. */
	
	/* TODO: 여기에 코드를 작성하세요.
		* TODO: 프로세스 종료 메시지를 구현하세요 (프로젝트2/process_termination.html 참고).
		* TODO: 프로세스 리소스 정리를 이곳에 구현할 것을 권장합니다. */

	process_cleanup();
}

/* Free the current process's resources. */
/* 현재 프로세스의 리소스를 해제합니다. */  

// 설명 : 프로세스를 깨끗히 청소합니다. 
// 인자 : void
// 반환 : 없음

static void
process_cleanup(void)
{
	struct thread *curr = thread_current();

#ifdef VM
	supplemental_page_table_kill(&curr->spt);
#endif

	uint64_t *pml4;
	/* Destroy the current process's page directory and switch back
	 * to the kernel-only page directory. */

	/* 현재 프로세스의 페이지 디렉토리를 파괴하고 
 	* 커널 전용 페이지 디렉토리로 전환합니다. */

	pml4 = curr->pml4;
	if (pml4 != NULL)
	{
		/* Correct ordering here is crucial.  We must set
		 * cur->pagedir to NULL before switching page directories,
		 * so that a timer interrupt can't switch back to the
		 * process page directory.  We must activate the base page
		 * directory before destroying the process's page
		 * directory, or our active page directory will be one
		 * that's been freed (and cleared). */
		/* 여기서 올바른 순서는 매우 중요합니다.
		* 프로세스 페이지 디렉토리로 다시 전환되지 않도록 
		* 타이머 인터럽트가 발생하기 전에 cur->pagedir를 NULL로 설정해야 합니다.
		* 프로세스의 페이지 디렉토리를 파괴하기 전에 기본 페이지 디렉토리를 활성화해야 합니다.
		* 그렇지 않으면 활성화된 페이지 디렉토리가 이미 해제(및 초기화)된 디렉토리가 될 수 있습니다. */

		curr->pml4 = NULL;
		pml4_activate(NULL);
		pml4_destroy(pml4);
	}
}

/* Sets up the CPU for running user code in the nest thread.
 * This function is called on every context switch. */
/* 다음 스레드에서 사용자 코드를 실행할 수 있도록 CPU를 설정합니다.
 * 이 함수는 컨텍스트 전환 시마다 호출됩니다. */

// 설명 : 프로세스 활성화하기 
// 인자 : 구조체 스레드 포인터 타입의 next (다음 스레드?)
// 반환 : 없음 
void process_activate(struct thread *next)
{
	/* Activate thread's page tables. */
	pml4_activate(next->pml4);

	/* Set thread's kernel stack for use in processing interrupts. */
	tss_update(next);
}

/* We load ELF binaries.  The following definitions are taken
 * from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
#define EI_NIDENT 16

#define PT_NULL 0			/* Ignore. */
#define PT_LOAD 1			/* Loadable segment. */
#define PT_DYNAMIC 2		/* Dynamic linking info. */
#define PT_INTERP 3			/* Name of dynamic loader. */
#define PT_NOTE 4			/* Auxiliary info. */
#define PT_SHLIB 5			/* Reserved. */
#define PT_PHDR 6			/* Program header table. */
#define PT_STACK 0x6474e551 /* Stack segment. */

#define PF_X 1 /* Executable. */
#define PF_W 2 /* Writable. */
#define PF_R 4 /* Readable. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
 * This appears at the very beginning of an ELF binary. */
struct ELF64_hdr
{
	unsigned char e_ident[EI_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct ELF64_PHDR
{
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

/* Abbreviations */
#define ELF ELF64_hdr
#define Phdr ELF64_PHDR

static bool setup_stack(struct intr_frame *if_);
static bool validate_segment(const struct Phdr *, struct file *);
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
						 uint32_t read_bytes, uint32_t zero_bytes,
						 bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
 * Stores the executable's entry point into *RIP
 * and its initial stack pointer into *RSP.
 * Returns true if successful, false otherwise. */
static bool
load(const char *file_name, struct intr_frame *if_)
{
	struct thread *t = thread_current();
	struct ELF ehdr;
	struct file *file = NULL;
	off_t file_ofs;
	bool success = false;
	int i;

	/* Allocate and activate page directory. */
	t->pml4 = pml4_create();
	if (t->pml4 == NULL)
		goto done;
	process_activate(thread_current());

	/* Open executable file. */
	file = filesys_open(file_name);
	if (file == NULL)
	{
		printf("load: %s: open failed\n", file_name);
		goto done;
	}

	/* Read and verify executable header. */
	if (file_read(file, &ehdr, sizeof ehdr) != sizeof ehdr || memcmp(ehdr.e_ident, "\177ELF\2\1\1", 7) || ehdr.e_type != 2 || ehdr.e_machine != 0x3E // amd64
		|| ehdr.e_version != 1 || ehdr.e_phentsize != sizeof(struct Phdr) || ehdr.e_phnum > 1024)
	{
		printf("load: %s: error loading executable\n", file_name);
		goto done;
	}

	/* Read program headers. */
	file_ofs = ehdr.e_phoff;
	for (i = 0; i < ehdr.e_phnum; i++)
	{
		struct Phdr phdr;

		if (file_ofs < 0 || file_ofs > file_length(file))
			goto done;
		file_seek(file, file_ofs);

		if (file_read(file, &phdr, sizeof phdr) != sizeof phdr)
			goto done;
		file_ofs += sizeof phdr;
		switch (phdr.p_type)
		{
		case PT_NULL:
		case PT_NOTE:
		case PT_PHDR:
		case PT_STACK:
		default:
			/* Ignore this segment. */
			break;
		case PT_DYNAMIC:
		case PT_INTERP:
		case PT_SHLIB:
			goto done;
		case PT_LOAD:
			if (validate_segment(&phdr, file))
			{
				bool writable = (phdr.p_flags & PF_W) != 0;
				uint64_t file_page = phdr.p_offset & ~PGMASK;
				uint64_t mem_page = phdr.p_vaddr & ~PGMASK;
				uint64_t page_offset = phdr.p_vaddr & PGMASK;
				uint32_t read_bytes, zero_bytes;
				if (phdr.p_filesz > 0)
				{
					/* Normal segment.
					 * Read initial part from disk and zero the rest. */
					read_bytes = page_offset + phdr.p_filesz;
					zero_bytes = (ROUND_UP(page_offset + phdr.p_memsz, PGSIZE) - read_bytes);
				}
				else
				{
					/* Entirely zero.
					 * Don't read anything from disk. */
					read_bytes = 0;
					zero_bytes = ROUND_UP(page_offset + phdr.p_memsz, PGSIZE);
				}
				if (!load_segment(file, file_page, (void *)mem_page,
								  read_bytes, zero_bytes, writable))
					goto done;
			}
			else
				goto done;
			break;
		}
	}

	/* Set up stack. */
	// 한비 퀸
	if (!setup_stack(if_))
		goto done;

	/* Start address. */
	if_->rip = ehdr.e_entry;

	/* TODO: Your code goes here.
	 * TODO: Implement argument passing (see project2/argument_passing.html). */

	success = true;

done:
	/* We arrive here whether the load is successful or not. */
	file_close(file);
	return success;
}

/* Checks whether PHDR describes a valid, loadable segment in
 * FILE and returns true if so, false otherwise. */
static bool
validate_segment(const struct Phdr *phdr, struct file *file)
{
	/* p_offset and p_vaddr must have the same page offset. */
	if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
		return false;

	/* p_offset must point within FILE. */
	if (phdr->p_offset > (uint64_t)file_length(file))
		return false;

	/* p_memsz must be at least as big as p_filesz. */
	if (phdr->p_memsz < phdr->p_filesz)
		return false;

	/* The segment must not be empty. */
	if (phdr->p_memsz == 0)
		return false;

	/* The virtual memory region must both start and end within the
	   user address space range. */
	if (!is_user_vaddr((void *)phdr->p_vaddr))
		return false;
	if (!is_user_vaddr((void *)(phdr->p_vaddr + phdr->p_memsz)))
		return false;

	/* The region cannot "wrap around" across the kernel virtual
	   address space. */
	if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
		return false;

	/* Disallow mapping page 0.
	   Not only is it a bad idea to map page 0, but if we allowed
	   it then user code that passed a null pointer to system calls
	   could quite likely panic the kernel by way of null pointer
	   assertions in memcpy(), etc. */
	if (phdr->p_vaddr < PGSIZE)
		return false;

	/* It's okay. */
	return true;
}

#ifndef VM
/* Codes of this block will be ONLY USED DURING project 2.
 * If you want to implement the function for whole project 2, implement it
 * outside of #ifndef macro. */

/* load() helpers. */
static bool install_page(void *upage, void *kpage, bool writable);

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment(struct file *file, off_t ofs, uint8_t *upage,
			 uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
	ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT(pg_ofs(upage) == 0);
	ASSERT(ofs % PGSIZE == 0);

	file_seek(file, ofs);
	while (read_bytes > 0 || zero_bytes > 0)
	{
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* Get a page of memory. */
		uint8_t *kpage = palloc_get_page(PAL_USER);
		if (kpage == NULL)
			return false;

		/* Load this page. */
		if (file_read(file, kpage, page_read_bytes) != (int)page_read_bytes)
		{
			palloc_free_page(kpage);
			return false;
		}
		memset(kpage + page_read_bytes, 0, page_zero_bytes);

		/* Add the page to the process's address space. */
		if (!install_page(upage, kpage, writable))
		{
			printf("fail\n");
			palloc_free_page(kpage);
			return false;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a minimal stack by mapping a zeroed page at the USER_STACK */
static bool
setup_stack(struct intr_frame *if_)
{
	uint8_t *kpage;
	bool success = false;

	kpage = palloc_get_page(PAL_USER | PAL_ZERO);
	if (kpage != NULL)
	{
		success = install_page(((uint8_t *)USER_STACK) - PGSIZE, kpage, true);
		if (success)
			if_->rsp = USER_STACK;
		else
			palloc_free_page(kpage);
	}
	return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
 * virtual address KPAGE to the page table.
 * If WRITABLE is true, the user process may modify the page;
 * otherwise, it is read-only.
 * UPAGE must not already be mapped.
 * KPAGE should probably be a page obtained from the user pool
 * with palloc_get_page().
 * Returns true on success, false if UPAGE is already mapped or
 * if memory allocation fails. */
static bool
install_page(void *upage, void *kpage, bool writable)
{
	struct thread *t = thread_current();

	/* Verify that there's not already a page at that virtual
	 * address, then map our page there. */
	return (pml4_get_page(t->pml4, upage) == NULL && pml4_set_page(t->pml4, upage, kpage, writable));
}
#else
/* From here, codes will be used after project 3.
 * If you want to implement the function for only project 2, implement it on the
 * upper block. */

static bool
lazy_load_segment(struct page *page, void *aux)
{
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
}

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment(struct file *file, off_t ofs, uint8_t *upage,
			 uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
	ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT(pg_ofs(upage) == 0);
	ASSERT(ofs % PGSIZE == 0);

	while (read_bytes > 0 || zero_bytes > 0)
	{
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		void *aux = NULL;
		if (!vm_alloc_page_with_initializer(VM_ANON, upage,
											writable, lazy_load_segment, aux))
			return false;

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a PAGE of stack at the USER_STACK. Return true on success. */
static bool
setup_stack(struct intr_frame *if_)
{
	bool success = false;
	void *stack_bottom = (void *)(((uint8_t *)USER_STACK) - PGSIZE);

	/* TODO: Map the stack on stack_bottom and claim the page immediately.
	 * TODO: If success, set the rsp accordingly.
	 * TODO: You should mark the page is stack. */
	/* TODO: Your code goes here */

	return success;
}
#endif /* VM */
