#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <stack>
#include <vector>

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

  stack<int> inodeStack;
  inodeStack.push(UFS_ROOT_DIRECTORY_INODE_NUMBER);

  // need to somehow store the current path of the directory we are in

  while (!inodeStack.empty()) {
    int inodeNumber = inodeStack.top();
    inodeStack.pop();

    inode_t inode;
    fs->stat(inodeNumber, &inode);

    if (inode.type == UFS_DIRECTORY) {
      dir_ent_t entries[inode.size / sizeof(dir_ent_t)];
      fs->read(inodeNumber, &entries, inode.size);

      sort(entries, entries + (inode.size / sizeof(dir_ent_t)), [](dir_ent_t a, dir_ent_t b) {
        return strcmp(a.name, b.name) < 0;
      });

      cout << "Directory /" << endl;

      for (dir_ent_t entry : entries) {
        cout << entry.inum << '\t' << entry.name << endl;
        if (entry.inum != -1) {
          inode_t entryInode;
          fs->stat(entry.inum, &entryInode);
          if (entryInode.type == UFS_DIRECTORY && strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0 ) {  
            inodeStack.push(entry.inum);
          }
        }
      }
    }
  }

  return 0;
}
