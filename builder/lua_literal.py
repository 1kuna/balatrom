"""Bounded literal-table parser. Never evaluate or execute Lua.

Accept only `return { ... }`, including nested literal tables, named/indexed
fields and array entries. Dynamic expressions, function calls and assignments
are rejected. Unsupported real localization formats need an explicit adapter.
"""
import re
from .archive import InputError

TOKEN = re.compile(r"\s+|--[^\n]*|(?P<string>\"(?:[^\"\\]|\\.)*\"|'(?:[^'\\]|\\.)*')|"
                   r"(?P<number>-?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)|"
                   r"(?P<word>[A-Za-z_][A-Za-z_0-9]*)|(?P<punct>[{}\[\]=,;])", re.DOTALL)
LONG_START = re.compile(r"\[(=*)\[")
QUOTED_STRING = re.compile(r'"(?:[^"\\]|\\.)*"|\'(?:[^\'\\]|\\.)*\'', re.DOTALL)
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")


def string_value(token):
    body = token[1:-1]
    out = []
    i = 0
    escapes = {"n": "\n", "r": "\r", "t": "\t", "\\": "\\", '"': '"', "'": "'"}
    while i < len(body):
        c = body[i]
        if c != "\\":
            out.append(c)
            i += 1
            continue
        i += 1
        if i >= len(body) or body[i] not in escapes:
            raise InputError("Unsupported Lua string escape")
        out.append(escapes[body[i]])
        i += 1
    return "".join(out)


def parse(data):
    if len(data) > 8 * 1024 * 1024:
        raise InputError("Literal table exceeds limit")
    try:
        source = data.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        raise InputError("Literal table must be UTF-8") from exc
    # Long strings/comments are deliberately not silently consumed.
    if re.search(r"\[(?:=*)\[", source):
        raise InputError("Lua long strings/comments are not supported")
    tokens = []
    pos = 0
    while pos < len(source):
        m = TOKEN.match(source, pos)
        if not m:
            raise InputError("Unsupported Lua syntax")
        pos = m.end()
        if m.lastgroup:
            tokens.append((m.lastgroup, m.group()))
        if len(tokens) > 200000:
            raise InputError("Literal token count exceeds limit")
    index = 0

    def take(expected=None):
        nonlocal index
        if index >= len(tokens):
            raise InputError("Unexpected end of literal table")
        token = tokens[index]
        if expected is not None and token[1] != expected:
            raise InputError("Expected literal token " + expected)
        index += 1
        return token

    def peek():
        return tokens[index][1] if index < len(tokens) else None

    def value(depth=0):
        if depth > 32:
            raise InputError("Literal nesting exceeds limit")
        kind, raw = take()
        if raw == "{":
            fields = {}
            next_array = 1
            while peek() != "}":
                if peek() == "[":
                    take("[")
                    key = value(depth + 1)
                    if not isinstance(key, (str, int)) or isinstance(key, bool):
                        raise InputError("Unsupported literal table key")
                    take("]")
                    take("=")
                    child = value(depth + 1)
                elif (index + 1 < len(tokens) and tokens[index][0] == "word"
                      and tokens[index + 1][1] == "="):
                    key = take()[1]
                    take("=")
                    child = value(depth + 1)
                else:
                    key = next_array
                    next_array += 1
                    child = value(depth + 1)
                if key in fields:
                    raise InputError("Duplicate literal table key")
                fields[key] = child
                if peek() in (",", ";"):
                    take()
                elif peek() != "}":
                    raise InputError("Expected literal table separator")
            take("}")
            return fields
        if kind == "string":
            return string_value(raw)
        if kind == "number":
            return float(raw) if any(c in raw for c in ".eE") else int(raw)
        if raw in ("true", "false", "nil"):
            return {"true": True, "false": False, "nil": None}[raw]
        raise InputError("Only literal values are supported; Lua is never executed")

    take("return")
    result = value()
    if not isinstance(result, dict) or index != len(tokens):
        raise InputError("Expected only one returned literal table")
    return result


def parse_assignment(data, path):
    """Read one static named initializer from a source file, never execute it.

    Locate a tokenized `A.B = {literal...}` outside strings/comments. Reject
    ambiguous assignments, selected dynamic values and trailing expressions.
    Unrelated executable source is not evaluated or copied into output.
    """
    if (not isinstance(path, list) or not 1 <= len(path) <= 8
            or any(not isinstance(p, str) or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", p) for p in path)):
        raise InputError("Invalid static assignment path")
    if len(data) > 8 * 1024 * 1024:
        raise InputError("Static source exceeds limit")
    try:
        source = data.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        raise InputError("Static source must be UTF-8") from exc
    tokens = []
    pos = 0
    while pos < len(source):
        if source[pos].isspace():
            pos += 1
            continue
        comment = source.startswith("--", pos)
        start = pos + 2 if comment else pos
        long = LONG_START.match(source, start)
        if long:
            close = "]" + long[1] + "]"
            end = source.find(close, long.end())
            if end < 0:
                raise InputError("Unclosed Lua long string/comment")
            if not comment:
                tokens.append((source[pos:end + len(close)], pos, end + len(close)))
            pos = end + len(close)
            continue
        if comment:
            end = source.find("\n", pos)
            pos = len(source) if end < 0 else end + 1
            continue
        if source[pos] in "\"'":
            quoted = QUOTED_STRING.match(source, pos)
            if not quoted:
                raise InputError("Unclosed Lua string")
            end = quoted.end()
            tokens.append((source[pos:end], pos, end))
            pos = end
        else:
            word = IDENTIFIER.match(source, pos)
            end = word.end() if word else pos + 1
            tokens.append((source[pos:end], pos, end))
            pos = end
        if len(tokens) > 400000:
            raise InputError("Static source token count exceeds limit")
    target = []
    for component in path:
        if target:
            target.append(".")
        target.append(component)
    candidates = []
    for i in range(len(tokens) - len(target)):
        if [t[0] for t in tokens[i:i + len(target)]] != target:
            continue
        if i and tokens[i - 1][0] in (".", ":"):
            continue
        at = i + len(target)
        if tokens[at][0] == "=" and (at + 1 == len(tokens) or tokens[at + 1][0] != "="):
            candidates.append(at + 1)
    if len(candidates) != 1:
        raise InputError("Static assignment must be unique")
    first = candidates[0]
    if first >= len(tokens) or tokens[first][0] != "{":
        raise InputError("Selected assignment is not a literal table")
    depth = 0
    for last in range(first, len(tokens)):
        token = tokens[last][0]
        if token == "{":
            depth += 1
        elif token == "}":
            depth -= 1
            if depth == 0:
                if last + 1 < len(tokens) and tokens[last + 1][0] in (".", "+", "-", "*", "/", "(", ":", "["):
                    raise InputError("Selected initializer has a trailing expression")
                literal = source[tokens[first][1]:tokens[last][2]].encode("utf-8")
                return parse(b"return " + literal)
    raise InputError("Unclosed selected literal table")
