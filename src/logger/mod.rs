use std::path::PathBuf;
use std::fs::{File, OpenOptions};
use std::io::Write;
use std::fmt;

#[derive(Clone)]
pub struct Logger {
    logger_file_path: PathBuf,
    enabled: bool
}

enum LoggerType {
    ERROR,
    INFO,
    WARNING
}

impl fmt::Display for LoggerType {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            LoggerType::INFO => write!(f, "INFO"),
            LoggerType::WARNING => write!(f, "WARNING"),
            LoggerType::ERROR => write!(f, "ERROR")
        }
    }
}

impl Logger {
    pub fn new(file_path: &str) -> Self {
        let logger_file_path = PathBuf::from(file_path);
        let mut enabled = true;

        match logger_file_path.try_exists() {
            Ok(false) => {
                if let Err(_) = File::create(&logger_file_path) {
                    enabled = false;
                }
            }
            
            Ok(true) | Err(_) => {}
        }

        Self { logger_file_path, enabled }
    }

    fn log(&self, error_type: Option<LoggerType>, message: &str) {
        if self.enabled != true {
            return;
        }

        let file = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .append(true)
            .open(&self.logger_file_path);

        // @TODO @UNHANDLED: handle open error if occurs

        if let Ok(mut file) = file {
            // @INVESTIGATE: should the file be locked while writing?
            // @TODO @UNHANDLED:  research how to handle the error here if needed
            file.write(
                format!(
                    "[{}]: {}\n",
                    error_type.unwrap_or(LoggerType::INFO),
                    message
                ).as_bytes()
            ).unwrap();
        }
    }

    pub fn error(&self, message: &str) {
        self.log(Some(LoggerType::ERROR), message);
    }
    
    pub fn warning(&self, message: &str) {
        self.log(Some(LoggerType::WARNING), message);
    }

    pub fn info(&self, message: &str) {
        self.log(None, message);
    }
}
