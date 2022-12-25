use std::sync::Arc;
use std::io::{stdin, Read};

use lsp::logger::Logger;
use lsp::request_handler::RequestHandler;

fn main() {
    let logger = Arc::new(Logger::new("/home/ng/_Projects/lsp/log.txt"));
    loop {
        let stdin = stdin();

        // reading the content length line.
        let mut content_length = String::new();
        stdin.read_line(&mut content_length).unwrap();

        // reading the empty new lines.
        let mut void = String::new();
        stdin.read_line(&mut void).unwrap();

        // reading the acctual request.
        if !content_length.is_empty() {
            if let Some(content_length_bytes_str) = content_length.split(':').last() {
                let logger = Arc::clone(&logger);

                let bytes: u64 = content_length_bytes_str.trim().parse::<u64>().unwrap_or(0);
                let mut handle = stdin.take(bytes);
                let mut buf = vec![0; bytes.try_into().unwrap()];
                handle.read(&mut buf).unwrap();

                if let Ok(request_str) = String::from_utf8(buf) {
                    RequestHandler::new(request_str, logger)
                        .handle();
                } else {
                    logger.error("Error in parsing request from bytes to string");
                }
            }
        }
    }
}
