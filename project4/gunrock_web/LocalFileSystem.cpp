#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <assert.h>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

void updateInode(LocalFileSystem *fs, int inodeNumber, inode_t inode);
int writeGeneral(LocalFileSystem *fs, int inodeNumber, const void *buffer, int size);
int claimFreeInode(LocalFileSystem *fs);
int claimFreeDataBlock(LocalFileSystem *fs);

LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {
  char *buffer = new char[UFS_BLOCK_SIZE];
  disk->readBlock(0, buffer);
  memcpy(super, buffer, sizeof(super_t));
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  inode_t inode;
  int ret = stat(parentInodeNumber, &inode);
  if (ret != 0) {
    return ret;
  }
  if (inode.type != UFS_DIRECTORY) {
    return EINVALIDINODE;
  }
  dir_ent_t entries[inode.size / sizeof(dir_ent_t)];
  read(parentInodeNumber, &entries, inode.size);
  for (dir_ent_t entry : entries) {
    if (strcmp(entry.name, name.c_str()) == 0) {
      return entry.inum;
    }
  }
  return ENOTFOUND;
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
  super_t super;
  readSuperBlock(&super);

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
  inode_t parentInode;

  // intial argument check
  int ret = stat(parentInodeNumber, &parentInode);
  if (ret != 0) {
    return ret;
  }
  if (parentInode.type != UFS_DIRECTORY) {
    return EINVALIDINODE;
  }
  if (name.size() > DIR_ENT_NAME_SIZE) {
    return EINVALIDNAME;
  }

  // name check
  dir_ent_t entries[parentInode.size / sizeof(dir_ent_t) + 1];
  read(parentInodeNumber, &entries, parentInode.size);
  for (dir_ent_t entry : entries) {
    if (strcmp(entry.name, name.c_str()) == 0) {
      if (entry.inum < 0) {
        return EINVALIDINODE;
      }
      inode_t existingInode;
      ret = stat(entry.inum, &existingInode);
      if (ret != 0) {
        return ret;
      }
      if (existingInode.type != type) {
        return EINVALIDTYPE;
      }
      return entry.inum;
    }
  }

  // find the first 0 in the inode bitmap
  int newInodeNumber = claimFreeInode(this);
  if (newInodeNumber == -1) {
    return ENOTENOUGHSPACE;
  }

  // add new entry to parent directory entries
  dir_ent_t newEntry;
  strcpy(newEntry.name, name.c_str());
  newEntry.inum = newInodeNumber;
  entries[parentInode.size / sizeof(dir_ent_t)] = newEntry;

  // update the parent inode
  parentInode.size += sizeof(dir_ent_t);
  updateInode(this, parentInodeNumber, parentInode);
  
  writeGeneral(this, parentInodeNumber, &entries, parentInode.size + sizeof(dir_ent_t));

  if (type == UFS_DIRECTORY) {
    inode_t newInode;
    newInode.type = UFS_DIRECTORY;
    newInode.size = 2 * sizeof(dir_ent_t);

    // add . and .. entries
    dir_ent_t newEntries[2];
    strcpy(newEntries[0].name, ".");
    newEntries[0].inum = newInodeNumber;
    strcpy(newEntries[1].name, "..");
    newEntries[1].inum = parentInodeNumber;

    // write entries into a new data block
    int newBlock = claimFreeDataBlock(this);
    if (newBlock == -1) {
      return ENOTENOUGHSPACE;
    }
    writeGeneral(this, newInodeNumber, &newEntries, newInode.size); // the problematic line

    updateInode(this, newInodeNumber, newInode);

  } else if (type == UFS_REGULAR_FILE) {
    inode_t newInode;
    newInode.type = UFS_REGULAR_FILE;
    newInode.size = 0;

    int newBlock = claimFreeDataBlock(this);
    if (newBlock == -1) {
      return ENOTENOUGHSPACE;
    }

    newInode.direct[0] = newBlock;

    updateInode(this, newInodeNumber, newInode);

  } else {
    return EINVALIDTYPE;
  }

  return newInodeNumber;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  inode_t inode;
  int ret = stat(inodeNumber, &inode);
  if (ret != 0) {
    return ret;
  }
  if (inode.type != UFS_REGULAR_FILE) {
    return EINVALIDTYPE;
  }

  if (size < 0 || size > MAX_FILE_SIZE) {
    return EINVALIDSIZE;
  }

  super_t super;
  readSuperBlock(&super);

  if (!diskHasSpace(&super, 0, inode.size - size, 0)) {
    return ENOTENOUGHSPACE;
  }

  return writeGeneral(this, inodeNumber, buffer, size);
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  return 0;
}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  for (int i = 0; i < super->inode_bitmap_len; i++) {
    char *buffer = new char[UFS_BLOCK_SIZE];
    disk->readBlock(super->inode_bitmap_addr + i, buffer);
    memcpy(inodeBitmap + i * UFS_BLOCK_SIZE, buffer, UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  for (int i = 0; i < super->inode_bitmap_len; i++) {
    char *buffer = new char[UFS_BLOCK_SIZE];
    memcpy(buffer, inodeBitmap + i * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE);
    disk->writeBlock(super->inode_bitmap_addr + i, buffer);
  }
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  for (int i = 0; i < super->data_bitmap_len; i++) {
    char *buffer = new char[UFS_BLOCK_SIZE];
    disk->readBlock(super->data_bitmap_addr + i, buffer);
    memcpy(dataBitmap + i * UFS_BLOCK_SIZE, buffer, UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
  for (int i = 0; i < super->data_bitmap_len; i++) {
    char *buffer = new char[UFS_BLOCK_SIZE];
    memcpy(buffer, dataBitmap + i * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE);
    disk->writeBlock(super->data_bitmap_addr + i, buffer);
  }
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) { 
  for (int i = 0; i < super->inode_region_len; i++) {
    char *buffer = new char[UFS_BLOCK_SIZE];
    disk->readBlock(super->inode_region_addr + i, buffer);
    memcpy(inodes + i * (UFS_BLOCK_SIZE / sizeof(inode_t)), buffer, UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
  for (int i = 0; i < super->inode_region_len; i++) {
    char *buffer = new char[UFS_BLOCK_SIZE];
    memcpy(buffer, inodes + i * (UFS_BLOCK_SIZE / sizeof(inode_t)), UFS_BLOCK_SIZE);
    disk->writeBlock(super->inode_region_addr + i, buffer);
  }
}

bool LocalFileSystem::diskHasSpace(super_t *super, int numInodesNeeded, int numDataBytesNeeded, int numDataBlocksNeeded) {
  unsigned char inodeBitmap[super->inode_bitmap_len * UFS_BLOCK_SIZE];
  readInodeBitmap(super, inodeBitmap);
  int numInodes = 0;
  for (int i = 0; i < super->num_inodes; i++) {
    if ((inodeBitmap[i / 8] & (1 << (i % 8))) == 0) {
      numInodes++;
    }
  }
  if (numInodes < numInodesNeeded) {
    return false;
  }

  unsigned char dataBitmap[super->data_bitmap_len * UFS_BLOCK_SIZE];
  readDataBitmap(super, dataBitmap);
  int numDataBlocks = 0;
  for (int i = 0; i < super->num_data; i++) {
    if ((dataBitmap[i / 8] & (1 << (i % 8))) == 0) {
      numDataBlocks++;
    }
  }
  if (numDataBlocks < ((numDataBytesNeeded + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE) + numDataBlocksNeeded) {
    return false;
  }

  return true;
}

// replaces an inode in the inode region
void updateInode(LocalFileSystem *fs, int inodeNumber, inode_t inode) {
  super_t super;
  fs->readSuperBlock(&super);
  inode_t inodes[UFS_BLOCK_SIZE * super.inode_region_len / sizeof(inode_t)];
  fs->readInodeRegion(&super, inodes);
  inodes[inodeNumber] = inode;
  fs->writeInodeRegion(&super, inodes);
}

int writeGeneral(LocalFileSystem *fs, int inodeNumber, const void *buffer, int size) {
  inode_t inode;
  int ret = fs->stat(inodeNumber, &inode);
  if (ret != 0) {
    return ret;
  }

  int bytesWritten = 0;
  int blockNumber = 0;
  char *blockBuffer = new char[UFS_BLOCK_SIZE];

  while (bytesWritten < size) {
    int bytesToWrite = min(size - bytesWritten, UFS_BLOCK_SIZE);
    memcpy(blockBuffer, (char *)buffer + bytesWritten, bytesToWrite);

    int blockAddr = inode.direct[blockNumber];
    if (blockAddr == -1) {
      int freeDataBlock = claimFreeDataBlock(fs);
      if (freeDataBlock == -1) {
        return ENOTENOUGHSPACE;
      }
      inode.direct[blockNumber] = freeDataBlock;
      updateInode(fs, inodeNumber, inode);
    }

    fs->disk->writeBlock(inode.direct[blockNumber], blockBuffer);
    bytesWritten += bytesToWrite;
    blockNumber++;
  }

  return size;
}

int claimFreeInode(LocalFileSystem *fs) {
  super_t super;
  fs->readSuperBlock(&super);
  unsigned char inodeBitmap[super.inode_bitmap_len * UFS_BLOCK_SIZE];
  fs->readInodeBitmap(&super, inodeBitmap);
  for (int i = 0; i < super.num_inodes; i++) {
    if ((inodeBitmap[i / 8] & (1 << (i % 8))) == 0) {
      inodeBitmap[i / 8] |= (1 << (i % 8));
      fs->writeInodeBitmap(&super, inodeBitmap);
      return i;
    }
  }
  return -1;
}

int claimFreeDataBlock(LocalFileSystem *fs) {
  super_t super;
  fs->readSuperBlock(&super);
  unsigned char dataBitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
  fs->readDataBitmap(&super, dataBitmap);
  for (int i = 0; i < super.num_data; i++) {
    if ((dataBitmap[i / 8] & (1 << (i % 8))) == 0) {
      dataBitmap[i / 8] |= (1 << (i % 8));
      fs->writeDataBitmap(&super, dataBitmap);
      return i + super.data_region_addr;
    }
  }
  return -1;
}