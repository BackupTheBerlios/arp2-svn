#define _KERNEL
#include <ixemul.h>
#include <ix.h>

#ifdef __pos__

static ix_segment *setup_segment_info(void)
{
  struct pOS_SegmentInfo SI = { sizeof(struct pOS_SegmentInfo) };
  usetup;
  
  u.u_segment_info.type = IX_SEG_TYPE_UNKNOWN;
  if (pOS_GetSegmentPtrInfo(u.u_segment_list, u.u_segment_ptr, NULL, &SI))
  {
    switch (SI.segi_HunkType)
    {
      case HUNKTYP_Code:
	u.u_segment_info.type = IX_SEG_TYPE_TEXT;
	break;
      case HUNKTYP_Data:
	u.u_segment_info.type = IX_SEG_TYPE_DATA;
	break;
      case HUNKTYP_Bss:
	u.u_segment_info.type = IX_SEG_TYPE_BSS;
	break;
    }
    u.u_segment_info.start = (void *)SI.segi_StartAddress;
    u.u_segment_info.size = (unsigned long)SI.segi_SegmSize;
  }
  return &u.u_segment_info;
}

ix_segment *ix_get_first_segment(long seglist)
{
  usetup;

  u.u_segment_list = (struct pOS_SegmentLst *)seglist;
  u.u_segment_ptr = &u.u_segment_list->sel_Seg;
  return setup_segment_info();
}

ix_segment *ix_get_next_segment(void)
{
  usetup;

  if ((u.u_segment_ptr = u.u_segment_ptr->seg_Next))
    return setup_segment_info();
  return NULL;
}

#else

static void setup_segment_info(void)
{
  usetup;
  
  u.u_segment_info.type = u.u_segment_no + 1;
  if (u.u_segment_info.type > IX_SEG_TYPE_MAX)
    u.u_segment_info.type = IX_SEG_TYPE_UNKNOWN;
  u.u_segment_info.start = (void *)(u.u_segment_ptr + 4);
  u.u_segment_info.size = *(unsigned long *)(u.u_segment_ptr - 4) - 8;
}

ix_segment *ix_get_first_segment(long seglist)
{
  usetup;

  u.u_segment_no = 0;
  u.u_segment_ptr = seglist << 2;
  setup_segment_info();
  return &u.u_segment_info;
}

ix_segment *ix_get_next_segment(void)
{
  usetup;

  u.u_segment_no++;
  u.u_segment_ptr = (*(long *)u.u_segment_ptr) << 2;
  if (u.u_segment_ptr == NULL)
    return NULL;
  setup_segment_info();
  return &u.u_segment_info;
}

#endif
