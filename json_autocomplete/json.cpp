#include <string>
#include <vector>
#include "json.h"

// Abstract base class for all parsers
struct Parser
{
    // true if this parser must consume at least one character
    virtual bool must_consume()
    {
        return true;
    }

    // true if this parser can consume the next character
    virtual bool matches(char next_char) = 0;

    // consume and autocomplete (in-place) the given prefix starting at given position
    virtual void autocomplete(std::string &prefix, std::size_t &position) = 0;
};

// A parser that references another parser. Useful for recursive structures.
struct Reference : Parser
{
    Parser *child;
    bool must_consume_value;

    Reference(Parser *child = nullptr, bool must_consume = true) : child(child), must_consume_value(must_consume) {}

    bool must_consume() override
    {
        return must_consume_value;
    }

    bool matches(char next_char) override
    {
        return child->matches(next_char);
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        child->autocomplete(prefix, position);
    }
};

// A parser that matches a literal string of any length.
struct Lit : Parser
{
    std::string literal;

    Lit(std::string literal) : literal(literal)
    {
        if (literal.empty())
            throw std::invalid_argument("Literal cannot be empty");
    }

    // must_consume is true, since the literal must consume at least one character

    bool matches(char next_char) override
    {
        return literal[0] == next_char;
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        for (std::size_t i = 0; i < literal.size(); i++)
        {
            if (position >= prefix.size())
                // missing character, auto-insert it!
                prefix.push_back(literal[i]);
            position++;
        }
    }
};

// A parser that matches a range of characters (ASCII).
struct Range : Parser
{
    char start;
    char end;
    char default_char;

    Range(char start, char end) : Range(start, end, start) {}
    Range(char start, char end, char default_char) : start(start), end(end), default_char(default_char)
    {
        if (start > end)
            throw std::invalid_argument("Range is empty");
    }

    bool matches(char next_char) override
    {
        return start <= next_char && next_char <= end;
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        if (position >= prefix.size())
            // missing character, auto-insert it!
            prefix.push_back(default_char);
        position++;
    }
};

// A parser that matches any character in a whitelist.
struct Any : Parser
{
    std::string whitelist;
    char default_char;

    Any(std::string whitelist) : Any(whitelist, whitelist[0]) {}
    Any(std::string whitelist, char default_char) : whitelist(whitelist), default_char(default_char)
    {
        if (whitelist.empty())
            throw std::invalid_argument("Whitelist cannot be empty");
    }

    bool matches(char next_char) override
    {
        return whitelist.find(next_char) != std::string::npos;
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        if (position >= prefix.size())
            // missing character, auto-insert it!
            prefix.push_back(default_char);
        position++;
    }
};

// A parser that matches any character not in a blacklist.
struct Except : Parser
{
    std::string blacklist;

    Except(std::string blacklist) : blacklist(blacklist)
    {
        if (blacklist.empty())
            throw std::invalid_argument("Blacklist cannot be empty");
    }

    bool matches(char next_char) override
    {
        return blacklist.find(next_char) == std::string::npos;
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        if (position < prefix.size())
            position++;
    }
};

// Makes the child parser optional.
struct Opt : Parser
{
    Parser *child;

    Opt(Parser *child) : child(child) {}

    bool must_consume() override
    {
        return false;
    }

    bool matches(char next_char) override
    {
        return child->matches(next_char);
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        if (position < prefix.size() && child->matches(prefix[position]))
            child->autocomplete(prefix, position);
    }
};

// Makes the child parser repeatable ANY number of times, including zero.
struct Rep : Parser
{
    Parser *child;

    Rep(Parser *child) : child(child) {}

    bool must_consume() override
    {
        return false;
    }

    bool matches(char next_char) override
    {
        return true;
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        while (position < prefix.size() && child->matches(prefix[position]))
            child->autocomplete(prefix, position);
    }
};

// Pick one of the children in order, and if none match, auto-fill first one.
struct Or : Parser
{
    std::vector<Parser *> children;

    Or(std::initializer_list<Parser *> children) : children(children)
    {
        if (this->children.empty())
            throw std::invalid_argument("Or must have at least one child");
    }

    bool must_consume() override
    {
        for (Parser *child : children)
            if (!child->must_consume())
                return false;
        return true;
    }

    bool matches(char next_char) override
    {
        for (Parser *child : children)
            if (child->matches(next_char))
                return true;
        return false;
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        if (position < prefix.size())
        {
            for (Parser *child : children)
                if (child->matches(prefix[position]))
                {
                    child->autocomplete(prefix, position);
                    return;
                }
        }
        // no child matched, auto-fill first one
        children[0]->autocomplete(prefix, position);
    }
};

// Concatenate the children in order.
struct Seq : Parser
{
    std::vector<Parser *> children;

    Seq(std::initializer_list<Parser *> children) : children(children)
    {
        if (this->children.empty())
            throw std::invalid_argument("Seq must have at least one child");
    }

    bool must_consume() override
    {
        for (Parser *child : children)
            if (child->must_consume())
                return true;
        return false;
    }

    bool matches(char next_char) override
    {
        for (Parser *child : children)
        {
            bool m = child->matches(next_char);
            if (m || child->must_consume())
                return m;
        }
        return false;
    }

    void autocomplete(std::string &prefix, std::size_t &position) override
    {
        for (Parser *child : children)
            child->autocomplete(prefix, position);
    }
};

Parser *create_json_parser()
{
    // primitives
    auto SingleDigit = new Range{'0', '9'};
    auto OptionalDigits = new Rep{SingleDigit};
    auto Quote = new Lit{"\""};
    auto WS = new Rep{new Any{" \n\r\t"}};
    auto Digits = new Seq{SingleDigit, OptionalDigits};

    auto Number = new Seq{
        new Opt{new Lit{"-"}}, // if missing, skip
        new Or{
            new Lit{"0"},
            new Seq{
                new Range{'1', '9'},
                OptionalDigits,
            },
        },
        new Opt{new Seq{
            new Lit{"."},
            Digits,
        }},
        new Opt{new Seq{
            new Or{
                new Lit{"e"},
                new Lit{"E"},
            },
            new Opt{new Or{
                new Lit{"+"},
                new Lit{"-"},
            }},
            Digits,
        }},
    };

    auto HexDigit = new Or{
        SingleDigit,
        new Range{'a', 'f'},
        new Range{'A', 'F'},
    };
    auto Unicode = new Seq{
        new Lit{"u"},
        HexDigit,
        HexDigit,
        HexDigit,
        HexDigit,
    };
    auto String = new Seq{
        Quote,
        new Rep{new Or{
            new Except{"\"\\"},
            new Seq{
                new Lit{"\\"},
                new Or{
                    new Any{"\"\\/bfnrt"},
                    Unicode,
                },
            },
        }},
        Quote,
    };

    auto Value = new Reference{};
    auto Object = new Reference{};
    auto Array = new Reference{};

    Value->child = new Seq{
        new Or{
            new Lit{"null"},
            String,
            Number,
            Object,
            Array,
            new Lit{"true"},
            new Lit{"false"},
        },
        WS,
    };

    auto KeyValue = new Seq{
        String,
        WS,
        new Lit{":"},
        WS,
        Value,
    };
    auto MemberList = new Seq{
        KeyValue,
        new Rep{new Seq{
            new Lit{","},
            WS,
            KeyValue,
        }},
    };
    Object->child = new Seq{
        new Lit{"{"},
        WS,
        new Opt{MemberList},
        new Lit{"}"},
    };

    auto ValueList = new Seq{
        Value,
        new Rep{new Seq{
            new Lit{","},
            WS,
            Value,
        }},
    };
    Array->child = new Seq{
        new Lit{"["},
        WS,
        new Opt{ValueList},
        new Lit{"]"},
    };

    auto WSValue = new Seq{WS, Value};

    return WSValue;
}

Parser *json_parser = create_json_parser();

std::string json_autocomplete_cpp(std::string prefix)
{
    std::size_t position = 0;
    json_parser->autocomplete(prefix, position);
    if (position != prefix.size())
        throw std::runtime_error("Given prefix is not from a valid JSON string!");
    return prefix;
}
