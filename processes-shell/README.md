# Wish Shell
A simple Unix shell implementation with support for basic commands, parallel execution, and I/O redirection.

## What is Wish?
Wish (Wisconsin Shell) is a command-line interpreter that can:
- Execute programs from configurable paths
- Run commands in parallel using `&`
- Redirect output to files using `>`
- Read commands from files (batch mode)

## Features
- **Built-in Commands**: `cd`, `exit`, `path`
- **Parallel Execution**: Run multiple commands simultaneously with `&`
- **Output Redirection**: Redirect command output to files with `>`
- **Interactive & Batch Mode**: Run interactively or from a script file
- **Error Handling**: Clear error messages for invalid commands

## Quick Start

### 1. Compile the Shell
```bash
gcc -o wish wish.c Vector.c -Wall
```

### 2. Run Interactive Mode
```bash
./wish
Wish> ls
Wish> echo hello world
Wish> exit
```

### 3. Run Batch Mode
```bash
./wish script.sh
```

## Built-in Commands

### `exit`
Exit the shell.
```bash
Wish> exit
```
- Takes no arguments
- Exits immediately

### `cd <directory>`
Change the current working directory.
```bash
Wish> cd /home/user
Wish> cd ..
```
- Takes exactly one argument (the directory path)
- Shows error if directory doesn't exist

### `path <dir1> <dir2> ...`
Set the search paths for executable programs.
```bash
Wish> path /bin /usr/bin
Wish> path /bin /usr/bin /usr/local/bin
Wish> path
```
- Takes zero or more arguments
- Default path is `/bin`
- With no arguments, clears all paths (no commands will work except built-ins)

## Features

### Parallel Execution
Run multiple commands in parallel using `&`:
```bash
Wish> cmd1 & cmd2 & cmd3
```
- All commands run simultaneously
- Shell waits for all to complete before showing next prompt

### Output Redirection
Redirect command output to a file using `>`:
```bash
Wish> ls > output.txt
Wish> echo hello world > greeting.txt
```
- Creates the file if it doesn't exist
- Overwrites the file if it exists
- Only one redirection per command allowed
- Must be at the end: `command > file`

### Combining Features
```bash
Wish> cmd1 > out1.txt & cmd2 > out2.txt
```
Run commands in parallel with individual output files.

## Usage Examples

### Basic Commands
```bash
Wish> ls
file1.txt  file2.txt
Wish> pwd
/home/user
Wish> echo Hello, World!
Hello, World!
```

### Change Directory
```bash
Wish> cd /tmp
Wish> pwd
/tmp
```

### Configure Paths
```bash
Wish> path /bin /usr/bin /usr/local/bin
Wish> ls
```

### Output Redirection
```bash
Wish> ls > files.txt
Wish> cat files.txt
file1.txt
file2.txt
```

### Parallel Execution
```bash
Wish> sleep 3 & sleep 2 & echo done
done
(waits 3 seconds total, not 5)
```

### Batch Mode
Create a file `script.sh`:
```bash
cd /tmp
echo Starting...
ls > files.txt
cat files.txt
exit
```

Run it:
```bash
./wish script.sh
```

## Error Handling

The shell prints `An error has occurred\n` for:
- Invalid number of arguments to built-in commands
- Failed `cd` to non-existent directory
- Command not found in any path
- Invalid redirection syntax (multiple `>`, wrong position)
- File operations that fail

### Common Errors

**Command not found:**
```bash
Wish> nonexistent
An error has occurred
```
Fix: Check if the program exists in your paths

**Invalid cd:**
```bash
Wish> cd
An error has occurred
Wish> cd dir1 dir2
An error has occurred
```
Fix: Use exactly one argument

**Invalid redirection:**
```bash
Wish> ls > file1.txt > file2.txt
An error has occurred
Wish> ls >
An error has occurred
Wish> > output.txt ls
An error has occurred
```
Fix: Use `command > file` format only

**Exit with arguments:**
```bash
Wish> exit now
An error has occurred
```
Fix: Use `exit` with no arguments

## How It Works

```
Input → Parse → Fork → Execute → Wait → Output
```

1. **Read Input**: Get command line from user or file
2. **Parse**: Split into commands and arguments, handle `&` and `>`
3. **Fork**: Create child process for each command
4. **Execute**: Search paths and run the program
5. **Wait**: Parent waits for all children to complete
6. **Repeat**: Show prompt and get next command

## Implementation Details

### Command Parsing
- Automatically adds spaces around `>`, `<`, `&` for proper parsing
- Splits input by spaces to get tokens
- Groups commands separated by `&`

### Path Resolution
- Searches directories in order specified by `path` command
- Checks if file exists and is executable
- Uses first match found

### Process Management
- Uses `fork()` to create child processes
- Parent process waits for all children using `waitpid()`
- Each command runs in its own process

## Requirements
- GCC compiler
- POSIX-compliant system (Linux, macOS, Unix)
- Vector.h/Vector.c helper files

Compile with: `gcc -o wish wish.c Vector.c -Wall`

## Limitations
- No support for pipes (`|`)
- No input redirection (`<`)
- No environment variable expansion
- No command history
- Maximum 1024 parallel commands
- No job control (background jobs)

## Tips

**Set up common paths:**
```bash
Wish> path /bin /usr/bin /usr/local/bin
```

**Test in batch mode first:**
- Create a script file to test complex sequences
- Easier to debug than typing repeatedly

**Parallel execution:**
- Use `&` to speed up independent operations
- Each command has its own process

**Check your paths:**
```bash
Wish> path
Wish> path /bin
```

## Project Structure
```
wish.c       - Main shell implementation
Vector.h     - Dynamic string array header
Vector.c     - Dynamic string array implementation
```

## Common Issues

**Segmentation fault?**
- Check Vector implementation
- Ensure proper memory allocation
- Verify file operations

**Commands not found?**
- Check current path: `path`
- Verify executable exists: `ls -la /bin/ls`
- Set correct paths: `path /bin /usr/bin`

**Parallel execution not working?**
- Ensure `&` is separated by spaces
- Check if all commands are valid

## Thread Safety
✓ Each command runs in separate process  
✓ Parent waits for all children  
✓ No shared memory between processes  
✓ Safe for parallel execution
