! void *CallInNewStack(int **psp, int *spnew, void (*proc)(), void *ptr);

! i0 : puntero a un sp.
! i1 : nuevo sp
! i2 : proc
! i3 : argumento para proc
.text
	.align 4
	.global _CallInNewStack
	.global _ChangeToStack

_CallInNewStack:
	save %sp, -96, %sp
	st %fp,[%sp-4]       ! %sp[-1]= %fp (frame pointer)
	st %i7,[%sp-8]       ! %sp[-2]= %i7 (direccion de retorno)
	st %sp,[%i0]         ! *psp= %sp
        mov %i3,%g1          ! %g1= args
	mov %i2,%g2          ! %g2= proc
                             ! Vamos a cambiar el stack
        and  %i1,-8,%i1      ! Para que quede multiplo de 8
	save %i1,-64,%sp     ! (%sp,%fp)= (&spnew[-24], %sp)
	ta 3                 ! lleva las ventanas de registro a memoria
	save %sp,-64,%sp     ! 4 ventanas libres
	save %sp,-64,%sp     ! aun cuando haya un "underflow" de ventanas
	save %sp,-64,%sp     ! no se llegara a la ventana del que
	save %sp,-96,%sp     ! llama a CallInNewStack

	call %g2             ! (*proc)(ptr)
	mov %g1,%o0

! void *ChangeToStack( int **psp, int **pspnew, void *prt);
! i0 : puntero a un sp
! i1 : nuevo sp
! i2 : valor retornado

_ChangeToStack:
	save %sp, -96, %sp
	ta 3                 ! lleva las ventanas de registro a memoria
	st %fp,[%sp-4]       ! %sp[-1]= %fp   (frame pointer)
	st %i7,[%sp-8]       ! %sp[-2]= %i7   (direccion de retorno)
	st %sp,[%i0]         ! *psp= %sp      (sp actual)
        ld [%i1],%i1         ! %i1= *pspnew
	ld [%i1-8],%i7       ! %i7= spnew[-2] (direccion de retorno)
	ld [%i1-4],%fp       ! %fp= spnew[-1] (frame pointer)
	mov %i1,%sp          ! %sp= spnew     (nuevo sp)
	mov %i2,%i0          ! %i0= ptr
	ret                  ! return ptr
	restore

