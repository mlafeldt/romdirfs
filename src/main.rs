use std::env;
use std::fs;
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
    let file = fs::File::open(&path).unwrap();
    let archive = romdir::RomdirArchive::new(std::io::BufReader::new(file)).unwrap();

    for f in archive.files.iter() {
        println!("{:10} {:08x}-{:08x} {}", f.name, f.offset, f.offset + f.size, f.size);
    }
}
