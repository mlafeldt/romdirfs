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
pub struct RomdirArchive<T: io::Read + io::Seek> {
    reader: T,
    files: Vec<String>,
}

impl<T: io::Read + io::Seek> RomdirArchive<T> {
    pub fn new(mut reader: T) -> RomdirResult<RomdirArchive<T>> {
        let mut files = Vec::new();

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
            let xi_size = reader.read_u16::<LittleEndian>()?;
            let size = reader.read_u32::<LittleEndian>()?;

            println!("{:?} {} {} {}", name_raw, name, xi_size, size);

            if name != "-" {
                files.push(name);
            }
        }

        Ok(RomdirArchive { reader, files })
    }
}
