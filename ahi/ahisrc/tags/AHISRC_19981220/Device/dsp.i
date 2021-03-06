* $Id$
* $Log$
* Revision 4.4  1998/01/12 20:07:28  lcs
* More restruction, mixer in C added. (Just about to make fraction 32 bit!)
*
* Revision 4.3  1997/08/02 17:11:59  lcs
* Right. Now echo should work!
*
* Revision 4.2  1997/08/02 16:32:39  lcs
* Fixed a memory trashing error. Will change it yet again now...
*
* Revision 4.1  1997/04/02 22:29:53  lcs
* Bumped to version 4
*
* Revision 1.1  1997/03/24 12:43:09  lcs
* Initial revision
*
*

	IFND    AHI_DSP_I
AHI_DSP_I	SET	1

	include	exec/types.i

	STRUCTURE Echo,0
	ULONG	ahiecho_Delay
	FPTR	ahiecho_Code		;The echo routine
	Fixed	ahiecho_FeedbackDS	;Delayed signal to same channel
	Fixed	ahiecho_FeedbackDO	;Delayed signal to other channel
	Fixed	ahiecho_FeedbackNS	;Normal signal to same channel
	Fixed	ahiecho_FeedbackNO	;Normal signal to other channel
	Fixed	ahiecho_MixN		;Normal signal
	Fixed	ahiecho_MixD		;Delayed signal
	ULONG	ahiecho_Offset		;(&Buffer-&SrcPtr)/sizeof(ahiecho_Buffer[0])
	APTR	ahiecho_SrcPtr		;Pointer to &Buffer
	APTR	ahiecho_DstPtr		;Pointer to &(Buffer[Delay])
	APTR	ahiecho_EndPtr		;Pointer to address after buffer
	ULONG	ahiecho_BufferLength	;Delay buffer length in samples
	ULONG	ahiecho_BufferSize	;Delay buffer size in bytes
	LABEL	ahiecho_Buffer		;Delay buffer
	LABEL	Echo_SIZEOF

	ENDC
