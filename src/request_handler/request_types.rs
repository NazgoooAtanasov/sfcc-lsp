use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
struct ClientInfo {
    name: String,
    version: Option<String>
}

#[derive(Serialize, Deserialize)]
pub struct InitializeRequest {
    proccess_id: Option<i32>,
    client_info: Option<ClientInfo>,
    locale: Option<String>,
    root_path: Option<String>,
    root_uri: Option<String>,
    // initialization_options: LSPAny
    // capabilities: something idk
    // trace is some kind of enum
    // workspace_folders is a vect of someting
}
