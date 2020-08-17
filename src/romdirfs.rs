use fuse::{FileAttr, FileType, Filesystem, ReplyAttr, ReplyData, ReplyDirectory, ReplyEntry, Request};
use libc::ENOENT;
use std::ffi::OsStr;
use std::io::prelude::*;
use std::time::{Duration, UNIX_EPOCH};

const TTL: Duration = Duration::from_secs(1); // 1 second

const HELLO_DIR_ATTR: FileAttr = FileAttr {
    ino: 1,
    size: 0,
    blocks: 0,
    atime: UNIX_EPOCH, // 1970-01-01 00:00:00
    mtime: UNIX_EPOCH,
    ctime: UNIX_EPOCH,
    crtime: UNIX_EPOCH,
    kind: FileType::Directory,
    perm: 0o755,
    nlink: 2,
    uid: 501,
    gid: 20,
    rdev: 0,
    flags: 0,
};

const HELLO_TXT_CONTENT: &str = "Hello World!\n";

const HELLO_TXT_ATTR: FileAttr = FileAttr {
    ino: 2,
    size: 13,
    blocks: 1,
    atime: UNIX_EPOCH, // 1970-01-01 00:00:00
    mtime: UNIX_EPOCH,
    ctime: UNIX_EPOCH,
    crtime: UNIX_EPOCH,
    kind: FileType::RegularFile,
    perm: 0o644,
    nlink: 1,
    uid: 501,
    gid: 20,
    rdev: 0,
    flags: 0,
};

use crate::romdir::Archive;

pub struct RomdirFS<R: Read + Seek> {
    archive: Archive<R>,
}

impl<R: Read + Seek> RomdirFS<R> {
    pub fn new(archive: Archive<R>) -> Self {
        RomdirFS { archive }
    }
}

impl<R: Read + Seek> Filesystem for RomdirFS<R> {
    fn lookup(&mut self, _req: &Request, parent: u64, name: &OsStr, reply: ReplyEntry) {
        if parent == 1 && self.archive.files.contains_key(name.to_str().unwrap()) {
            reply.entry(&TTL, &HELLO_TXT_ATTR, 0);
        } else {
            reply.error(ENOENT);
        }
    }

    fn getattr(&mut self, _req: &Request, ino: u64, reply: ReplyAttr) {
        match ino {
            1 => reply.attr(&TTL, &HELLO_DIR_ATTR),
            2..=100 => reply.attr(&TTL, &HELLO_TXT_ATTR),
            _ => reply.error(ENOENT),
        }
    }

    fn read(&mut self, _req: &Request, ino: u64, _fh: u64, offset: i64, _size: u32, reply: ReplyData) {
        if ino > 1 {
            let name = &self.archive.file_names()[ino as usize - 1];
            let data = self.archive.read_file(name).unwrap();
            reply.data(&data);
        } else {
            reply.error(ENOENT);
        }
    }

    fn readdir(&mut self, _req: &Request, ino: u64, _fh: u64, offset: i64, mut reply: ReplyDirectory) {
        if ino != 1 {
            reply.error(ENOENT);
            return;
        }

        let mut entries = vec![(1, FileType::Directory, "."), (1, FileType::Directory, "..")];

        for (i, name) in self.archive.file_names_iter().enumerate() {
            entries.push((i + 1, FileType::RegularFile, name));
        }

        for (i, entry) in entries.into_iter().enumerate().skip(offset as usize) {
            reply.add(entry.0 as u64, (i + 1) as i64, entry.1, entry.2);
        }
        reply.ok();
    }
}
