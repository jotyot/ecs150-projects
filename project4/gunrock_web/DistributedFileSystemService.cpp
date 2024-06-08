#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>

#include "DistributedFileSystemService.h"
#include "ClientError.h"
#include "ufs.h"
#include "WwwFormEncodedDict.h"
#include <string.h>

using namespace std;

DistributedFileSystemService::DistributedFileSystemService(string diskFile) : HttpService("/ds3/") {
  this->fileSystem = new LocalFileSystem(new Disk(diskFile, UFS_BLOCK_SIZE));
}  

void DistributedFileSystemService::get(HTTPRequest *request, HTTPResponse *response) {
  vector<string> pathComponents = request->getPathComponents();
  if (pathComponents.size() < 1 || pathComponents[0] != "ds3") {
    throw ClientError::badRequest();
  }

  int currentInum = UFS_ROOT_DIRECTORY_INODE_NUMBER;

  for (unsigned int i = 1; i < pathComponents.size(); i++) {
    int nextInum = fileSystem->lookup(currentInum, pathComponents[i]);
    if (nextInum < 0) {
      throw ClientError::notFound();
    }
    currentInum = nextInum;
  }

  inode_t inode;
  fileSystem->stat(currentInum, &inode);

  if (inode.type == UFS_DIRECTORY) {
    string output;
    dir_ent_t entries[inode.size / sizeof(dir_ent_t)];
    fileSystem->read(currentInum, entries, inode.size);

    sort(entries + 2, entries + (inode.size / sizeof(dir_ent_t)), [](dir_ent_t a, dir_ent_t b) {
        return strcmp(a.name, b.name) < 0;
    });

    for (int i = 2; i < inode.size / (int)sizeof(dir_ent_t); i++) {
      inode_t entryInode;
      fileSystem->stat(entries[i].inum, &entryInode);
      output += entries[i].name;
      if (entryInode.type == UFS_DIRECTORY) {
        output += "/";
      }
      output += "\n";
    }
    response->setBody(output);
  } else {
    char buffer[inode.size];
    fileSystem->read(currentInum, buffer, inode.size);
    response->setBody(string(buffer, inode.size));
  }
}
  
void DistributedFileSystemService::put(HTTPRequest *request, HTTPResponse *response) {
  vector<string> pathComponents = request->getPathComponents();
  if (pathComponents.size() < 2 || pathComponents[0] != "ds3") {
    throw ClientError::badRequest();
  }

  int currentInum = UFS_ROOT_DIRECTORY_INODE_NUMBER;

  fileSystem->disk->beginTransaction();


  int type = UFS_DIRECTORY;
  for (unsigned int i = 1; i < pathComponents.size(); i++) {
    if (i == pathComponents.size() - 1) {
      type = UFS_REGULAR_FILE;
    }

    int nextInum = fileSystem->create(currentInum, type, pathComponents[i]);
    if (nextInum < 0) {
      fileSystem->disk->rollback();
      if (nextInum == -ENOTENOUGHSPACE) {
        throw ClientError::insufficientStorage();
      } else if (nextInum == -EINVALIDTYPE) {
        throw ClientError::conflict();
      } else {
        throw ClientError::badRequest();
      }
    }
    currentInum = nextInum;
  }

  string contents = request->getBody();

  int ret = fileSystem->write(currentInum, contents.c_str(), contents.size());
  if (ret == -ENOTENOUGHSPACE) {
    fileSystem->disk->rollback();
    throw ClientError::insufficientStorage();
  }

  fileSystem->disk->commit();

  response->setBody("");
}

void DistributedFileSystemService::del(HTTPRequest *request, HTTPResponse *response) {
  vector<string> pathComponents = request->getPathComponents();

  if (pathComponents.size() < 2 || pathComponents[0] != "ds3") {
    throw ClientError::badRequest();
  }

  int currentInum = UFS_ROOT_DIRECTORY_INODE_NUMBER;

  for (unsigned int i = 1; i < pathComponents.size() - 1; i++) {
    int nextInum = fileSystem->lookup(currentInum, pathComponents[i]);
    if (nextInum < 0) {
      throw ClientError::notFound();
    }
    currentInum = nextInum;
  }

  fileSystem->disk->beginTransaction();

  inode_t inode;
  fileSystem->stat(currentInum, &inode);

  int ret = fileSystem->unlink(currentInum, pathComponents[pathComponents.size() - 1]);
  if (ret < 0) {
    fileSystem->disk->rollback();
    if (ret == -ENOTENOUGHSPACE) {
      throw ClientError::insufficientStorage();
    } else {
      throw ClientError::badRequest();
    }
  }
  
  fileSystem->disk->commit();

  response->setBody("");
}
