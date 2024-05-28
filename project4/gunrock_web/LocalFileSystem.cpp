#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <assert.h>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;


LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {
  char *buffer = new char[UFS_BLOCK_SIZE];
  disk->readBlock(0, buffer);
  memcpy(super, buffer, sizeof(super_t));
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  return 0;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  super_t super;
  readSuperBlock(&super);
  if (inodeNumber < 0 || inodeNumber >= super.num_inodes) {
    return EINVALIDINODE;
  }
  char *buffer = new char[UFS_BLOCK_SIZE];
  int blockNumber = inodeNumber / (UFS_BLOCK_SIZE / sizeof(inode_t));
  disk->readBlock(super.inode_region_addr + blockNumber, buffer);
  memcpy(inode, buffer + inodeNumber * sizeof(inode_t), sizeof(inode_t));
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  inode_t inode;
  int ret = stat(inodeNumber, &inode);
  if (ret != 0) {
    return ret;
  }

  if (size < 0 || size > inode.size) {
    return EINVALIDSIZE;
  }

  int bytesRead = 0;
  int blockNumber = 0;
  char *blockBuffer = new char[UFS_BLOCK_SIZE];

  while (bytesRead < size) {
    disk->readBlock(inode.direct[blockNumber], blockBuffer);
    int bytesToRead = min(size - bytesRead, UFS_BLOCK_SIZE);
    memcpy((char *)buffer + bytesRead, blockBuffer, bytesToRead);
    bytesRead += bytesToRead;
    blockNumber++;
  }

  return size;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  return 0;
}
