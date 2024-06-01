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

  int newInode = fs->create(UFS_ROOT_DIRECTORY_INODE_NUMBER, UFS_DIRECTORY, "a");

  fs->create(newInode, UFS_DIRECTORY, "b");

  inode_t inode;
  fs->stat(newInode, &inode);

  cout << "inode type: " << (inode.type ? "file" : "directory") << endl;
  cout << "inode size: " << inode.size << endl;
  cout << "datablock: " << inode.direct[0] << endl;

  return 0;
}

