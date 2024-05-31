#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

bool allocated(LocalFileSystem *fs, int inodeNumber);

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cout << argv[0] << ": diskImageFile inodeNumber" << endl;
    return 1;
  }

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fs = new LocalFileSystem(disk);

  int inodeNumber = atoi(argv[2]);
  inode_t inode;
  fs->stat(inodeNumber, &inode);

  int numBlocks = (inode.size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;
  char buffer[UFS_BLOCK_SIZE * numBlocks];
  fs->read(inodeNumber, buffer, inode.size);

  cout << "File blocks" << endl;
  if (numBlocks == 0 && allocated(fs, inodeNumber)) {
    cout << inode.direct[0] << endl;
  }
  for (int i = 0; i < numBlocks; i++) {
    cout << inode.direct[i] << endl;
  }
  cout << endl;
  cout << "File data" << endl;
  cout.write(buffer, inode.size);
}

bool allocated(LocalFileSystem *fs, int inodeNumber) {
  super_t super;
  fs->readSuperBlock(&super);

  int block = inodeNumber / (UFS_BLOCK_SIZE * 8);
  unsigned char *bitmap = new unsigned char[super.inode_bitmap_len * UFS_BLOCK_SIZE];
  fs->readInodeBitmap(&super, bitmap);

  return bitmap[block] & (1 << (inodeNumber % (UFS_BLOCK_SIZE * 8)));
}