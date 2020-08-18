#![allow(dead_code)]

use byteorder::{LittleEndian, ReadBytesExt};
use std::collections::BTreeMap;
use std::fs;
use std::io::prelude::*;
use std::io::{self, SeekFrom};
use std::path::Path;
use thiserror::Error;

pub type RomdirResult<T> = Result<T, RomdirError>;

#[derive(Debug, Error)]
pub enum RomdirError {
    /// An error caused by I/O
    #[error(transparent)]
    Io(#[from] io::Error),

    /// The requested file could not be found in the archive
    #[error("specified file not found in archive")]
    FileNotFound,
}

#[derive(Clone, Debug)]
pub struct Archive<R: Read + Seek> {
    reader: R,
    pub files: BTreeMap<String, FileMetadata>,
}

#[derive(Copy, Clone, Debug)]
pub struct FileMetadata {
    pub offset: u32,
    pub size: u32,
}

impl<R: Read + Seek> Archive<R> {
    pub fn new(mut reader: R) -> RomdirResult<Archive<R>> {
        let mut files = BTreeMap::new();
        let mut offset = 0;

        loop {
            let mut name_raw = [0; 10];
            reader.read_exact(&mut name_raw)?;

            if files.is_empty() && &name_raw[0..6] != b"RESET\0" {
                reader.seek(SeekFrom::Current(6))?;
                continue;
            }

            if name_raw[0] == 0 {
                break;
            }

            let eos = name_raw.iter().position(|&x| x == 0).unwrap();
            let name = String::from_utf8_lossy(&name_raw[..eos]).into_owned();
            let _ = reader.read_u16::<LittleEndian>()?; // ignore xi_size
            let size = reader.read_u32::<LittleEndian>()?;

            if name != "-" {
                files.insert(name, FileMetadata { offset, size });
            }

            // offset must be aligned to 16 bytes
            offset += (size + 15) & !15;
        }

        Ok(Archive { reader, files })
    }

    pub fn file_names(&self) -> Vec<String> {
        self.files.keys().cloned().collect()
    }

    pub fn file_names_iter(&self) -> impl Iterator<Item = &str> {
        self.files.keys().map(|s| s.as_str())
    }

    pub fn extract_all<P: AsRef<Path>>(&mut self, dir: P) -> RomdirResult<()> {
        for name in &self.file_names() {
            let outpath = dir.as_ref().join(name);
            self.extract_file(name, outpath)?;
        }

        Ok(())
    }

    pub fn extract_file<P: AsRef<Path>>(&mut self, name: &str, path: P) -> RomdirResult<()> {
        let md = self.metadata(name)?;

        self.reader.seek(SeekFrom::Start(md.offset as u64))?;
        let mut take = self.reader.by_ref().take(md.size as u64);

        let mut outfile = fs::File::create(&path)?;
        io::copy(&mut take, &mut outfile)?;

        Ok(())
    }

    pub fn read_file(&mut self, name: &str) -> RomdirResult<Vec<u8>> {
        let md = self.metadata(name)?;

        self.reader.seek(SeekFrom::Start(md.offset as u64))?;
        let mut take = self.reader.by_ref().take(md.size as u64);

        let mut buf = Vec::with_capacity(md.size as usize);
        take.read_to_end(&mut buf)?;

        Ok(buf)
    }

    pub fn metadata(&self, name: &str) -> RomdirResult<FileMetadata> {
        match self.files.get(name) {
            Some(md) => Ok(*md),
            None => Err(RomdirError::FileNotFound),
        }
    }
}
