# blacklist

A command line program, that reads stdin and removes all lines contained the blacklist-file provided as an argument and finally prints the result to stdout.

Note that it may also be used as a whitelist with the -w flag.

Useful together with dmenu

## Installation

```
    make install
```
## Usage

```
    cat <list> | blacklist <blacklist-file>
```
For more info, see 
```
    blacklist -h
```

## Example

```
    find ~/Articles -name '*pdf'
         | blacklist ~/Articles/blacklist \
         | dmenu
```
