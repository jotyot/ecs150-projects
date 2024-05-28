#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fs = new LocalFileSystem(disk);

  super_t super;
  fs->readSuperBlock(&super);

  cout << "Super" << endl;
  cout << "inode_region_addr " << super.inode_region_addr << endl;
  cout << "data_region_addr " << super.data_region_addr << endl;
  cout << endl;

  cout << "Inode bitmap" << endl;
  unsigned char *inodeBitmap = new unsigned char[super.inode_bitmap_len * UFS_BLOCK_SIZE];
  fs->readInodeBitmap(&super, inodeBitmap);
  for (int i = 0; i < super.inode_bitmap_len * UFS_BLOCK_SIZE; i++) {
    cout << (unsigned int)inodeBitmap[i] << " ";
  }
  cout << endl << endl;

  cout << "Data bitmap" << endl;
  unsigned char *dataBitmap = new unsigned char[super.data_bitmap_len * UFS_BLOCK_SIZE];
  fs->readDataBitmap(&super, dataBitmap);
  for (int i = 0; i < super.data_bitmap_len * UFS_BLOCK_SIZE; i++) {
    cout << (unsigned int)dataBitmap[i] << " ";
  }
  cout << endl;
}
