use std::sync::Arc;
use crate::logger::Logger;
use crate::request_handler::request_types::InitializeRequest;

mod request_types;

pub struct RequestHandler {
    request_raw: String,
    logger: Arc<Logger>
}

impl RequestHandler {
    pub fn new(request_str: String, logger: Arc<Logger>) -> Self {
        Self {
            request_raw: request_str,
            logger
        }
    }

    pub fn handle(self) {
        std::thread::spawn(move || {
            let request_json: serde_json::Value = serde_json::from_str(&self.request_raw)
                .unwrap_or_else(|_| {
                    self.logger.error("Error in parsing the request to json");
                    serde_json::Value::Null
                });

            if request_json != serde_json::Value::Null {
                match request_json["method"].as_str() {
                    Some("initialize") => {
                        let params: InitializeRequest = serde_json::from_value(request_json["params"].clone())
                            .expect("Deffinetely rust problem, not mine");
                        self.logger.info("Request type initialize received");
                    }
                    Some(request_type) => {
                        self.logger.info(&format!("Request type {} is not yet implemented", request_type));
                    }
                    None => {
                        self.logger.error("Request type could not be recognized");
                    }
                }
            }
        });
    }
}
