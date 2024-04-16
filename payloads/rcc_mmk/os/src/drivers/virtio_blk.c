#include "drivers.h"
#include "efs.h"
#include "virtio.h"
#include "log.h"

const int R = 0;
const int W = 1;

void virtio_read_block(BlockCache *b) { 
  virtio_disk_rw(b, R); 
}

void virtio_write_block(BlockCache *b) {
  virtio_disk_rw(b, W); 
}

static BlockDevice BLOCK_DEVICE;

BlockDevice *virtio_block_device_init() {
  if(virtio_disk_init() != 0){
    return NULL;
  }
  BLOCK_DEVICE.read_block = virtio_read_block;
  BLOCK_DEVICE.write_block = virtio_write_block;
  return &BLOCK_DEVICE;
}
