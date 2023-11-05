'''
Using the auto-filling parsers from parser.py, we can define a JSON parser in a very concise way.
The grammar is mostly directly from https://www.json.org/json-en.html,
BUT, note that the parser assumes that in a "branch" of the grammar (Or, Opt, Rep), the correct path to take is always completely determined by the single next character, i.e. no look-ahead or backtracking is required.
While this is true for JSON, the way the diagrams are drawn on the website is a bit misleading, so trying to implement it exactly as it is on the website leads to errors.
More specifically, for whitespaces, we always make a construct (the CamelCase global variables below) start with a NON-whitespace, and usually end with a whitespace.
'''


from .parser import *


WS = Rep(Any([' ', '\n', '\r', '\t']))

Digit = Seq(Range('0', '9'), Rep(Range('0', '9')))
Number = Seq(
    Opt(Lit('-')), # if missing, skip
    Or( # Pick one of the following in order, and if none match, auto-fill first one
        Lit('0'), 
        Seq(
            Range('1', '9'),
            Rep(Range('0', '9'))
        )
    ),
    Opt(Seq(
        Lit('.'),
        Digit
    )),
    Opt(Seq(
        Or(
            Lit('e'),
            Lit('E')
        ),
        Opt(Or(
            Lit('+'),
            Lit('-')
        )),
        Digit
    ))
)

HexDigit = Or(
    Range('0', '9'),
    Range('a', 'f'),
    Range('A', 'F')
)
Unicode = Seq(Lit('u'), HexDigit, HexDigit, HexDigit, HexDigit)
String = Seq(
    Lit('"'),
    Rep(Or(
        Except(['"', '\\']),
        Seq(
            Lit('\\'),
            Or(
                Any(['"', '\\', '/', 'b', 'f', 'n', 'r', 't']),
                Unicode
            )
        )
    )),
    Lit('"')
)

# Forward references for recursive structures
Value = Reference(None)
Object = Reference(None)
Array = Reference(None)

Value.child = Seq(
    Or(
        Lit('null'),
        String,
        Number,
        Object,
        Array,
        Lit('true'),
        Lit('false')
    ),
    WS
)
Value.min_len = lambda: 1

KeyValue = Seq(
    String, WS,
    Lit(':'), WS,
    Value
)
MemberList = Seq(
    KeyValue,
    Rep(Seq(
        Lit(','), WS,
        KeyValue,
    ))
)
Object.child = Seq(
    Lit('{'), WS,
    Opt(MemberList),
    Lit('}')
)
Object.min_len = lambda: 2

ValueList = Seq(
    Value,
    Rep(Seq(
        Lit(','), WS,
        Value
    ))
)
Array.child = Seq(
    Lit('['), WS,
    Opt(ValueList),
    Lit(']')
)
Array.min_len = lambda: 2

WSValue = Seq(WS, Value)

def json_autocomplete(prefix: str) -> str:
    """Autocomplete any prefix of a JSON string in a minimal way. Returns the completed string."""
    completed, pos = WSValue(prefix, 0)
    assert pos == len(completed) # prefix must be fully consumed
    return completed