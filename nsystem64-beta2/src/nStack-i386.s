.text
	.align	4
	.global	_CallInNewStack
	.global	_ChangeToStack

# void *CallInNewStack(int **psp, int *spnew, void (*proc)(), void *ptr)
#  8(%esp) : puntero a un sp (psp)
# 12(%esp) : nuevo sp (pspnew)
# 16(%esp) : proc
# 20(%esp) : argumento para proc

_CallInNewStack:
	pushl %ebp
	movl %esp,%ebp
        pushl %edi
        pushl %esi
        pushl %edx
        pushl %ecx
        pushl %ebx
        movl 8(%ebp),%eax	# %eax = *psp
        movl %esp,(%eax)
        movl 12(%ebp),%ecx	# %ecx = pspnew
        movl %ecx,%esp
        movl 20(%ebp),%ebx	# %ebx = ptr
        pushl %ebx
        movl 16(%ebp),%edx	# %edx = proc
        call *%edx
#	popl %ebx
#	popl %ebx
#       popl %ecx
#       popl %edx
#	popl %ebp
	ret


# void *ChangeToStack( int **psp, int **pspnew, void *prt)
#  8(%esp) : puntero a un sp (psp)
# 12(%esp) : nuevo sp (pspnew)
# 16(%esp) : valor retornado (prt)

_ChangeToStack:

	pushl %ebp
	movl %esp,%ebp
        pushl %edi
        pushl %esi
        pushl %edx
        pushl %ecx
        pushl %ebx
	movl 8(%ebp),%edx
	movl %esp,(%edx)
        movl 12(%ebp),%eax
	movl (%eax),%esp
	movl 16(%ebp),%eax
	popl %ebx
        popl %ecx
        popl %edx
        popl %esi
        popl %edi
	popl %ebp
	ret
