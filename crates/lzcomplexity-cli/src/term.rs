//! Terminal colours and message formatting — mirrors the C++
//! `lz::standalone::print_msg` / colour constants in `config_utils.hpp`.

pub const RED: &str = "\x1b[1;31m";
pub const GREEN: &str = "\x1b[1;32m";
pub const YELLOW: &str = "\x1b[1;33m";
pub const BLUE: &str = "\x1b[1;34m";
pub const END: &str = "\x1b[0m";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Msg {
    Error,
    Info,
    Warning,
    Debug,
}

fn color(t: Msg) -> &'static str {
    match t {
        Msg::Error => RED,
        Msg::Warning => YELLOW,
        Msg::Info => GREEN,
        Msg::Debug => BLUE,
    }
}

fn header(t: Msg) -> &'static str {
    match t {
        Msg::Error => " [ Error ] ",
        Msg::Info => " [ Info ] ",
        Msg::Warning => " [ Warning ] ",
        Msg::Debug => " [ Debug ] ",
    }
}

/// Format a coloured, header-prefixed message. Continuation lines are indented
/// by the header width, matching the C++ `print_msg`.
pub fn print_msg(t: Msg, msg: &str) -> String {
    let h = header(t);
    let mut out = String::with_capacity(msg.len() + h.len() + 32);
    out.push_str(color(t));
    out.push_str(h);
    out.push_str(END);

    let mut first = true;
    for line in msg.split('\n') {
        if !first {
            out.push('\n');
            for _ in 0..h.len() {
                out.push(' ');
            }
        }
        out.push_str(line);
        first = false;
    }
    out
}
