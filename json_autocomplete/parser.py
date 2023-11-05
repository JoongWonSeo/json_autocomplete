'''
This is a general "consumer" + "auto-filler" parser that takes a prefix of a grammar, and attempts to "consume", i.e. traverse it char-by-char according to the defined grammar as much as possible, and when it reaches the end of the string, it auto-fills the rest of the grammar with the first possible option.
It's important to note that we assume that the prefix is of a completely valid string that adheres to the grammar, so there is no error handling!
This is useful if you are streaming a long string, e.g. JSON, and you want to parse it as you receive it, meaning you'd want to complete that string in a minimal way.
'''


class Parser:
    '''Base class for all parsers. A parser is a callable that takes a prefix and a position, and returns a completed prefix and latest consumed position.'''
    def min_len(self):
        return 1
    
    def matches(self, char):
        raise NotImplementedError

class Reference(Parser):
    '''A parser that references another parser. Useful for recursive structures.'''
    def __init__(self, child):
        self.child = child

    def min_len(self):
        return self.child.min_len()
    
    def matches(self, char):
        return self.child.matches(char)
    
    def __call__(self, prefix, pos):
        return self.child(prefix, pos)

class Lit(Parser):
    '''A parser that matches a literal string of any length.'''
    def __init__(self, value):
        self.value = value

    def min_len(self):
        return len(self.value)

    def matches(self, char):
        return char == self.value[0]

    def __call__(self, prefix, pos):
        for i in range(len(self.value)):
            if pos >= len(prefix):
                # missing char, auto-insert it!
                prefix += self.value[i]
            pos += 1
        return prefix, pos

class Range(Parser):
    '''A parser that matches a range of characters (ASCII).'''
    def __init__(self, start, end, default=None):
        self.start = start
        self.end = end
        self.default = start if default is None else default

    def matches(self, char):
        return self.start <= char <= self.end
    
    def __call__(self, prefix, pos):
        if pos >= len(prefix):
            # missing char, auto-insert it!
            prefix += self.default
            pos += len(self.default) - 1
        pos += 1
        return prefix, pos

class Any(Parser):
    '''A parser that matches any character in a whitelist.'''
    def __init__(self, whitelist, default=None):
        self.whitelist = whitelist
        self.default = whitelist[0] if default is None else default
        
    def matches(self, char):
        return char in self.whitelist
    
    def __call__(self, prefix, pos):
        if pos >= len(prefix):
            # missing char, auto-insert it!
            prefix += self.default
            pos += len(self.default) - 1
        pos += 1
        return prefix, pos

class Except(Parser):
    '''A parser that matches any character not in a blacklist.'''
    def __init__(self, blacklist, default=None):
        self.blacklist = blacklist
        self.default = default if default is None else ''

    def matches(self, char):
        return char not in self.blacklist
    
    def __call__(self, prefix, pos):
        if pos >= len(prefix):
            # missing char, auto-insert it!
            prefix += self.default
            pos += len(self.default) - 1
        pos += 1
        return prefix, pos

class Opt(Parser):
    '''Makes the child parser optional.'''
    def __init__(self, child):
        self.child = child

    def min_len(self):
        return 0

    def matches(self, char):
        return self.child.matches(char)

    def __call__(self, prefix, pos):
        if pos < len(prefix) and self.child.matches(prefix[pos]):
            prefix, pos = self.child(prefix, pos)
        return prefix, pos

class Rep(Parser):
    '''Makes the child parser repeatable ANY number of times, including zero.'''
    def __init__(self, child):
        self.child = child
    
    def min_len(self):
        return 0

    def matches(self, char):
        return True
    
    def __call__(self, prefix, pos):
        while pos < len(prefix) and self.child.matches(prefix[pos]):
            prefix, pos = self.child(prefix, pos)
        return prefix, pos

class Or(Parser):
    '''Pick one of the children in order, and if none match, auto-fill first one.'''
    def __init__(self, *children):
        self.children = children
    
    def min_len(self):
        return min(child.min_len() for child in self.children)

    def matches(self, char):
        return any(child.matches(char) for child in self.children)

    def __call__(self, prefix, pos):
        if pos < len(prefix):
            for child in self.children:
                if child.matches(prefix[pos]):
                    return child(prefix, pos)
        # no child matched, auto-insert first one
        return self.children[0](prefix, pos)

class Seq(Parser):
    '''Run the children in order.'''
    def __init__(self, *children):
        self.children = children

    def min_len(self):
        return sum(child.min_len() for child in self.children)

    def matches(self, char):
        for child in self.children:
            if (m:=child.matches(char)) or child.min_len() > 0:
                return m
        return False

    def __call__(self, prefix, pos):
        for child in self.children:
            prefix, pos = child(prefix, pos)
        return prefix, pos
