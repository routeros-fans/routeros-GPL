/* This is used to pack YAFFS2 tags, not YAFFS1tags. */

#ifndef __YAFFS_PACKEDTAGS2_H__
#define __YAFFS_PACKEDTAGS2_H__

#include "yaffs_guts.h"
#include "yaffs_ecc.h"
#include "yaffs_ecc_mlc.h"

typedef struct {
	unsigned sequenceNumber;
	unsigned objectId;
	unsigned chunkId;
	unsigned byteCount;
} yaffs_PackedTags2TagsPart;

typedef struct {
	yaffs_PackedTags2TagsPart t;
	yaffs_ECCOther ecc;
} yaffs_PackedTags2;

int yaffs_PackTags2(yaffs_PackedTags2 * pt, const yaffs_ExtendedTags * t,
		    unsigned maxPackedSize);
void yaffs_UnpackTags2(yaffs_ExtendedTags * t, yaffs_PackedTags2 * pt, int ignoreEcc);
#endif
