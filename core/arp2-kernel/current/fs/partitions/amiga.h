/*
 *  fs/partitions/amiga.h
 */

int amiga_partition(struct parsed_partitions *state, struct block_device *bdev);
int parse_amiga_partition(struct parsed_partitions *state, struct block_device *bdev, u32 offset);

