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
    name_map: HashMap<String, u64>,
}

impl<R: Read + Seek> RomdirFS<R> {
    pub fn new(archive: Archive<R>) -> Self {
        let mut ino_map = HashMap::new();
        let mut name_map = HashMap::new();

        for (i, name) in archive.file_names_iter().enumerate() {
            let ino = ROOT_INO + (i as u64) + 1;
            ino_map.insert(ino, name.to_string());
            name_map.insert(name.to_string(), ino);
        }

        RomdirFS {
            archive,
            ino_map,
            name_map,
        }
    }
}

impl<R: Read + Seek> Filesystem for RomdirFS<R> {
    fn lookup(&mut self, _req: &Request<'_>, parent: u64, name: &OsStr, reply: ReplyEntry) {
        if parent == ROOT_INO {
            if let Some(ino) = self.name_map.get(name.to_str().unwrap()) {
                let md = self.archive.metadata(name.to_str().unwrap()).unwrap();
                reply.entry(&TTL, &make_file_attr(*ino, md.size as u64), 0);
                return;
            }
        }
        reply.error(ENOENT);
    }

    fn getattr(&mut self, _req: &Request<'_>, ino: u64, reply: ReplyAttr) {
        if let Some(name) = self.ino_map.get(&ino) {
            let md = self.archive.metadata(name).unwrap();
            reply.attr(&TTL, &make_file_attr(ino, md.size as u64))
        } else if ino == ROOT_INO {
            reply.attr(&TTL, &make_dir_attr(ino));
        } else {
            reply.error(ENOENT);
        }
    }

    fn read(&mut self, _req: &Request<'_>, ino: u64, _fh: u64, offset: i64, size: u32, reply: ReplyData) {
        if let Some(name) = self.ino_map.get(&ino) {
            let data = self.archive.read_file(name).unwrap();
            reply.data(&data[offset as usize..offset as usize + size as usize]);
        } else {
            reply.error(ENOENT);
        }
    }

    fn readdir(&mut self, _req: &Request<'_>, ino: u64, _fh: u64, offset: i64, mut reply: ReplyDirectory) {
        if ino != ROOT_INO {
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
        ino,
        size,
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
        ino,
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
