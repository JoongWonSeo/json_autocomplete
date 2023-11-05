# JSON Autocomplete

This library offers a simple function to complete any prefix of a valid JSON string, in a way that makes it a valid JSON again (e.g. by closing all open brackets and quotes), in a minimal way.

There is only one function, `json_autocomplete(json_prefix: str) -> str`, which takes a prefix of a valid JSON string and returns a valid JSON string that is the shortest possible completion of the prefix.

The heck did I develop this for? When streaming a response from a LLM like ChatGPT, where the model generates a JSON string, you can render it before the model is done generating the response.

Another use case could be when you want to allow the user to enter a JSON string, but you want to offer autocomplete suggestions. You can use this function to get the shortest possible completion of the prefix the user has entered, and then offer that as a suggestion.

## Examples

```python
>>> json_autocomplete('')
'null'
>>> json_autocomplete('n')
'null'
>>> json_autocomplete('tr')
'true'
>>> json_autocomplete('-')
'-0'
>>> json_autocomplete('2.')
'2.0'
>>> json_autocomplete('[')
'[]'
>>> json_autocomplete('{')
'{}'
>>> json_autocomplete('{"')
'{"": null}'
>>> json_autocomplete('{"a": 1, "b": 2')
'{"a": 1, "b": 2}'
```

## Installation

```bash
pip install json-autocomplete
```

Then, simply import the function:

```python
from json_autocomplete import json_autocomplete

json_autocomplete('{"a": 1, "b": 2')
```