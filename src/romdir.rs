use byteorder::{LittleEndian, ReadBytesExt};
use std::io;
use thiserror::Error;

pub type RomdirResult<T> = Result<T, RomdirError>;

#[derive(Debug, Error)]
pub enum RomdirError {
    /// An error caused by I/O
    #[error(transparent)]
    Io(#[from] io::Error),
}

#[derive(Clone, Debug)]
pub struct Archive<T: io::Read + io::Seek> {
    reader: T,
    pub files: Vec<FileData>,
}

#[derive(Clone, Debug)]
pub struct FileData {
    pub name: String,
    pub offset: u32,
    pub size: u32,
}

impl<T: io::Read + io::Seek> Archive<T> {
    pub fn new(mut reader: T) -> RomdirResult<Archive<T>> {
        let mut files = Vec::new();
        let mut offset = 0;

        loop {
            let mut name_raw = [0; 10];
            reader.read_exact(&mut name_raw)?;

            if files.is_empty() && &name_raw[0..6] != b"RESET\0" {
                reader.seek(io::SeekFrom::Current(6))?;
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
                files.push(FileData { name, offset, size });
            }

            // offset must be aligned to 16 bytes
            offset += (size + 15) & !15;
        }

        Ok(Archive { reader, files })
    }
}
