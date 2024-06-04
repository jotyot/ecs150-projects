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

  int bInode = fs->create(newInode, UFS_DIRECTORY, "b");

  int fileInode = fs->create(bInode, UFS_REGULAR_FILE, "c.txt");

  char buffer[100];
  strcpy(buffer, "file contents");
  fs->write(fileInode, buffer, strlen(buffer));

  cout << fs->lookup(bInode, "a.txt") << endl;

  return 0;
}

