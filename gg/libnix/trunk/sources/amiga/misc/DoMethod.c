
#if defined( __mc68000__ )

asm("
		.text

		.globl	_DoMethod
		.globl	_DoMethodA


_DoMethod:	lea	sp@(8:W),a1
		jra	L_DoMethod

_DoMethodA:	movel	sp@(8:W),a1
L_DoMethod:	movel	sp@(4:W),d0
		beqs	L_Null
		movel	a2,sp@-
		movel	d0,a2
		movel	a2@(-4:W),a0
		jbsr	L_Invoke
		movel	sp@+,a2
L_Null:		rts

L_Invoke:	movel	a0@(8:W),sp@-
		rts
");

#else

#include <intuition/classes.h>
#include <clib/alib_protos.h>

ULONG
DoMethodA( Object *obj, Msg message )
{
  if( obj == NULL )
  {
    return 0;
  }
  else
  {
    return CallHookA( &OCLASS( obj )->cl_Dispatcher, obj, message );
  }
}

ULONG
DoMethod( Object *obj, ULONG methodID, ... )
{
  return DoMethodA( obj, (APTR) &methodID );
};

#endif
