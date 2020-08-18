use fuse::{FileAttr, FileType, Filesystem, ReplyAttr, ReplyData, ReplyDirectory, ReplyEntry, Request};
use libc::ENOENT;
use std::collections::HashMap;
use std::ffi::OsStr;
use std::io::prelude::*;
use std::time::{Duration, UNIX_EPOCH};

use crate::romdir::Archive;

const TTL: Duration = Duration::from_secs(60 * 60 * 24);
const ROOT_INO: u64 = 1;

pub struct RomdirFS<R: Read + Seek> {
    archive: Archive<R>,
    ino_map: HashMap<u64, String>,
}

impl<R: Read + Seek> RomdirFS<R> {
    pub fn new(archive: Archive<R>) -> Self {
        let mut ino_map = HashMap::new();

        for (i, name) in archive.file_names_iter().enumerate() {
            ino_map.insert(ROOT_INO + (i as u64) + 1, name.to_string());
        }

        RomdirFS { archive, ino_map }
    }
}

impl<R: Read + Seek> Filesystem for RomdirFS<R> {
    fn lookup(&mut self, _req: &Request, parent: u64, name: &OsStr, reply: ReplyEntry) {
        if parent == ROOT_INO && self.archive.files.contains_key(name.to_str().unwrap()) {
            let md = self.archive.metadata(name.to_str().unwrap()).unwrap();

            let mut ino = 0u64;
            for (i, n) in &self.ino_map {
                if name.to_str().unwrap() == n {
                    ino = *i;
                }
            }

            reply.entry(&TTL, &make_file_attr(ino, md.size as u64), 0);
        } else {
            reply.error(ENOENT);
        }
    }

    fn getattr(&mut self, _req: &Request, ino: u64, reply: ReplyAttr) {
        match ino {
            ino if self.ino_map.contains_key(&ino) => {
                let md = self.archive.metadata(self.ino_map.get(&ino).unwrap()).unwrap();
                reply.attr(&TTL, &make_file_attr(ino, md.size as u64))
            }
            ROOT_INO => reply.attr(&TTL, &make_dir_attr(ino)),
            _ => reply.error(ENOENT),
        }
    }

    fn read(&mut self, _req: &Request, ino: u64, _fh: u64, offset: i64, size: u32, reply: ReplyData) {
        if self.ino_map.contains_key(&ino) {
            let data = self.archive.read_file(self.ino_map.get(&ino).unwrap()).unwrap();
            reply.data(&data[offset as usize..offset as usize + size as usize]);
        } else {
            reply.error(ENOENT);
        }
    }

    fn readdir(&mut self, _req: &Request, ino: u64, _fh: u64, offset: i64, mut reply: ReplyDirectory) {
        if ino != 1 {
            reply.error(ENOENT);
            return;
        }

        let mut entries = vec![
            (ROOT_INO, FileType::Directory, "."),
            (ROOT_INO, FileType::Directory, ".."),
        ];

        for (i, name) in &self.ino_map {
            entries.push((*i, FileType::RegularFile, name));
        }

        for (i, entry) in entries.into_iter().enumerate().skip(offset as usize) {
            reply.add(entry.0 as u64, (i + 1) as i64, entry.1, entry.2);
        }
        reply.ok();
    }
}

fn make_file_attr(ino: u64, size: u64) -> FileAttr {
    FileAttr {
        ino: ino,
        size: size,
        blocks: 0,
        atime: UNIX_EPOCH,
        mtime: UNIX_EPOCH,
        ctime: UNIX_EPOCH,
        crtime: UNIX_EPOCH,
        kind: FileType::RegularFile,
        perm: libc::S_IFREG as u16 | 0o444,
        nlink: 1,
        uid: unsafe { libc::getuid() },
        gid: unsafe { libc::getgid() },
        rdev: 0,
        flags: 0,
    }
}

fn make_dir_attr(ino: u64) -> FileAttr {
    FileAttr {
        ino: ino,
        size: 0,
        blocks: 0,
        atime: UNIX_EPOCH,
        mtime: UNIX_EPOCH,
        ctime: UNIX_EPOCH,
        crtime: UNIX_EPOCH,
        kind: FileType::Directory,
        perm: libc::S_IFDIR as u16 | 0o555,
        nlink: 2,
        uid: unsafe { libc::getuid() },
        gid: unsafe { libc::getgid() },
        rdev: 0,
        flags: 0,
    }
}
