/*                                            */
/*                 AudioLabs                  */
/*    applications<>drivers communication:    */
/*              message structure             */
/*                Version 2.1                 */


#include "BSP.h"


/* (OBSOLETE) Commands */
#define AOD_CMD_INFO        1
#define AOD_CMD_QUIT        2
#define AOD_CMD_START       3
#define AOD_CMD_STOP        4
#define AOD_CMD_SETPARAMS   5
#define AOD_CMD_SETLRLEVELS 8


/* (OBSOLETE) Errors: general */
#define AOD_ERR_NOERROR     0
#define AOD_ERR_UNKNOWNCMD  1
#define AOD_ERR_HARDFAIL    2

/* (OBSOLETE) Errors: unsupported parameter values */
#define AOD_ERR_BUFFERSIZE  10
#define AOD_ERR_CHANNELS    11
#define AOD_ERR_DATATYPE    12

/* (OBSOLETE) Samplepoint format */
#define AOD_DAT_16LS        0
#define AOD_DAT_8LU         5




#define EIO_CMD_INFO            1
#define EIO_CMD_QUIT            2
#define EIO_CMD_PLAY            3
#define EIO_CMD_STOP            4
#define EIO_CMD_SETPARAMS       5
#define EIO_CMD_REC            10
#define EIO_CMD_RECPLAY        11
#define EIO_CMD_RECMONITOR     12
#define EIO_CMD_RECPLAYMONITOR 13


/* Errors: general */
#define EIO_ERR_NOERROR     0
#define EIO_ERR_UNKNOWNCMD  1
#define EIO_ERR_HARDFAIL    2

/* Errors: unsupported parameter values */
#define EIO_ERR_BUFFERSIZE  10
#define EIO_ERR_CHANNELS    11
#define EIO_ERR_DATATYPE    12

/* Samplepoint format */
#define EIO_DAT_16LS        0
#define EIO_DAT_8LU         5

/* OBSOLETE */
typedef struct {
        struct Message msg;             /* Exec message */
        char         mpchDrvName[12];   /* Driver name */
        LONG         lCommand;          /* Command */
        LONG         lError;            /* Error number */
        BYTE         *pbBuffer1;        /* Address of buffer 1 */
        BYTE         *pbBuffer2;        /* Address of buffer 2 */
        BYTE         *pbBuffer3;        /* PRIVATE */
        BYTE         *pbBuffer4;        /* PRIVATE */
        LONG         lBufferSize;       /* Size of buffer contents (in bytes) */
        WORD         wChannels;         /* Number of interleaved channels */
        WORD         wDataType;         /* Samplepoint format */
        ULONG        ulStereoPhase;     /* PRIVATE */
        ULONG        ulClock;           /* Signal clock */
        ULONG        ulLPCutoffFreq;    /* PRIVATE */
        ULONG        ulLeftLevel;       /* Output level for left channel  (0 = ignore; 1 = min; 0xFFFF = max) */
        ULONG        ulRightLevel;      /* Output level for right channel (0 = ignore; 1 = min; 0xFFFF = max) */
        BOOL         fOnePage;          /* Recording fits in one page (only buffer 1 is valid) */
        ULONG        ulSignalMask;      /* Swapper signal mask  */
        struct Task* ptaskSwapper;      /* Swapper task address */
        LONG         mpulLevels[32];    /* PRIVATE */
        LONG         mpulPans[32];      /* PRIVATE */
        LONG         mpulData[32];      /* PRIVATE */
        ULONG        mpulReserved[32];  /* PRIVATE */
        ULONG        ulActiveParams;    /* mask for active params */
        ULONG        mpulParams[32];    /* board specific params  */
} AnalogOutMessage;



typedef struct {
        struct Message msg;             /* Exec message */
        char         mpchDrvName[12];   /* Driver name */
        LONG         lCommand;          /* Command */
        LONG         lError;            /* Error number */
        BYTE         *pbBuffer1;        /* Address of playbuffer 1 */
        BYTE         *pbBuffer2;        /* Address of playbuffer 2 */
        BYTE         *pbBuffer3;        /* Address of recbuffer1 */
        BYTE         *pbBuffer4;        /* Address of recbuffer2 */
        LONG         lBufferSize;       /* Size of buffer contents (in bytes) */
        WORD         wChannels;         /* Number of interleaved channels */
        WORD         wDataType;         /* Samplepoint format */
        ULONG        ulRes1;            /* PRIVATE */
        ULONG        ulClock;           /* Signal clock */
        ULONG        ulRes2;            /* PRIVATE */
        ULONG        ulRes3;            /* PRIVATE */
        ULONG        ulRes4;            /* PRIVATE */
        BOOL         fOnePage;          /* Recording fits in one page (only buffer 1 is valid) */
        ULONG        ulSignalMask;      /* Swapper signal mask  */
        struct Task* ptaskSwapper;      /* Swapper task address */
        LONG         mplLevels[32];    /* PRIVATE */
        LONG         mplPans[32];      /* PRIVATE */
        LONG         mplData[32];      /* PRIVATE */
        ULONG        mplReserved[32];  /* PRIVATE */
        ULONG        ulActiveParams;    /* mask for active params */
        ULONG        mpulParams[32];    /* board specific params  */
} ExternalIOMessage;

