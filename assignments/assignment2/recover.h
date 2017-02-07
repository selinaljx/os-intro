typedef struct Param{
	char *dfn;
	char *recover;
	char *dest;
	char *cleanse;
} Param;

typedef struct Isset{
	int dfn;
	int info;
	int list;
	int recover;
	int output;
	int cleanse;
} Isset;

#pragma pack(push, 1)
typedef struct BootEntry{
	uint8_t BS_jmpBoot[3];	  /* Assembly instruction to jump to boot code */
	uint8_t BS_OEMName[8];    /* OEM Name in ASCII */
	uint16_t BPB_BytesPerSec;  /* Bytes per sector. Allowed values include 512, 1024, 2048 and 4096 */
	uint8_t BPB_SecPerClus;   /* Sectors per cluster(data unit). Allowed values are powers of 2, */
				  /* but the cluster size must be 32KB or smaller */
	uint16_t BPB_RsvdSecCnt;  /* Size in sectors of the reserved area */
	uint8_t BPB_NumFATs;	  /* Number of FATs */
	uint16_t BPB_RootEntCnt;  /* Maximum number of files in the root directory for FAT12 and FAT16. */
				  /* This is 0 for FAT32 */
	uint16_t BPB_TotSec16;    /* 16-bit value of number of sectors in file system */
	uint8_t BPB_Media; 	  /* Media type */
	uint16_t BPB_FATSz16;	  /* 16-bit size in sectors of each FAT for FAT12 and FAT16.*/
				  /* For FAT32, this field is 0 */
	uint16_t BPB_SecPerTrk;   /* Sectors per track of storage device */
	uint16_t BPB_NumHeads;	  /* Number of heads in storage device */
	uint32_t BPB_HiddSec; 	  /* Number of sectors before the start of partition */
	uint32_t BPB_TotSec32;	  /* 32-bit value of number of sectors in file system. */
				  /* Either this value or the 16-bit value above must be 0 */
	uint32_t BPB_FATSz32;	  /* 32-bit size in sectors of one FAT */
	uint16_t BPB_ExtFlags;    /* A flag for FAT */
	uint16_t BPB_FSVer;	  /* The major and minor version number */
	uint32_t BPB_RootClus;    /* Cluster where the root directory can be found */
	uint16_t BPB_FDInfo;	  /* Sector where FSINFO structure can be found */
	uint16_t BPB_BkBootSec;   /* Sector where backup copy of boot sector is located */
	uint8_t BPB_Reserved[12]; /* Reserved */
	uint8_t BS_DrvNum;	  /* BIOS INT13h drive number */
	uint8_t BS_Reserved1;	  /* Not used */	
	uint8_t BS_BootSig;	  /* Extended boot signature to identify if the next three values are valid */
	uint32_t BS_VolID;	  /* Volume serial number */
	uint8_t BS_VolLab[11];    /* Volume label in ASCII. User defines when creating the file system */
	uint8_t BS_FilSysType[8]; /* File system type label in ASCII */
} BootEntry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct DirEntry{
	uint8_t DIR_Name[11];	   /* File name */
	uint8_t DIR_Attr;	   /* File attributes */
	uint8_t DIR_NTRes;	   /* Reserved */
	uint8_t DIR_CrtTimeTenth;  /* Created time (tenths of second) */
	uint16_t DIR_CrtTime;	   /* Created time (hours, minutes, seconds) */
	uint16_t DIR_CrtDate;	   /* Created day */
	uint16_t DIR_LstAccDate;   /* Accessed day */
	uint16_t DIR_FstClusHI;	   /* High 2 bytes of the first cluster address */
	uint16_t DIR_WrtTime;	   /* Writted time (hours, minutes, seconds) */
	uint16_t DIR_WrtDate;	   /* Written day */
	uint16_t DIR_FstClusLO;	   /* Low 2 bytes of the first cluster address */
	uint32_t DIR_FileSize;	   /* File size in bytes. (0 for directories) */
} DirEntry;
#pragma pack(pop)
