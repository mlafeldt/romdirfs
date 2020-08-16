use std::env;
use std::fs;
use std::io;
use std::io::prelude::*;
use std::path::Path;
use std::process;

mod romdir;

fn main() {
    let args: Vec<_> = env::args().skip(1).collect();
    if args.len() != 2 {
        eprintln!("usage: romdirfs <file> <mountpoint>");
        process::exit(1);
    }

    let path = Path::new(&args[0]);
    let mut f = fs::File::open(&path).unwrap();
    let mut buf = Vec::with_capacity(4 * 1024 * 1024);
    f.read_to_end(&mut buf).unwrap();

    let mut archive = romdir::Archive::new(io::Cursor::new(buf)).unwrap();

    for (name, data) in archive.files.iter() {
        println!(
            "{:10} {:08x}-{:08x} {}",
            name,
            data.offset,
            data.offset + data.size,
            data.size
        );
    }

    archive.extract_all("/tmp/romdir").unwrap();
    archive.extract_file("VERSTR", "/tmp/verstr.bin").unwrap();
}
