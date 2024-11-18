#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);

// Argc와 Argv를 Parsing하기 위한 함수
void split_argument(char*, int*, char**);
// Stack에 값을 정렬 추가하기 위한 함수
void argument_stack(char**, int, struct intr_frame*);

#endif /* userprog/process.h */
