![Documentation Generator](https://github.com/cmannett85/malbolge/workflows/Documentation%20Generator/badge.svg) ![Release Builder](https://github.com/cmannett85/malbolge/workflows/Release%20Builder/badge.svg) ![Unit test coverage](https://img.shields.io/badge/Unit_Test_Coverage-97.5%25-brightgreen)

<img src="./playground/logo.svg" alt="Malbolge Logo" width="250"/>

# malbolge
A virtual machine to execute Malbolge programs, written in C++20.

## Usage
You can load from a file like this:
```
malbolge my_prog.mal
```
The file extension is ignored.  Or you can pipe output into it:
```
cat my_prog.mal | malbolge
```
As a final alternative that is useful for programmatic execution, you can use
the `--string` flag:
```
malbolge --string 'This will not compile...'
```
Like any terminal application, you access help with `--help`.  You can also get
log output (in stderr) via increasing the number of `-l` switches, e.g.:
```
$ malbolge -ll ./test/programs/hello_world.mal
2020-07-05 11:53:31.095433[INFO]: Loading file: "./test/programs/hello_world.mal"
2020-07-05 11:53:31.095504[DEBUG]: File size: 132
2020-07-05 11:53:31.095588[INFO]: File loaded
2020-07-05 11:53:31.095652[DEBUG]: Loaded size: 131
2020-07-05 11:53:31.122950[DEBUG]: Starting program
Hello World!
```
You can increase logging up to 3 levels.  As it is output into the error stream, you can split the logging output into a separate file to preserve the program output:
```
$ malbolge -lll ./test/programs/hello_world.mal  2> logging.txt
Hello World!
```

## Debugging
Primitive debugging is supported through the use of a debugging script specified by the `--debugger-script` flag, for example:
```
malbolge -ll --debugger-script my_prog.dbg my_prog.mal
```
The file extension is not relevant, it can be anything (or nothing).  The script is essentially a list of commands for the debugger, it's syntax and supported functions are listed below.

### Syntax
* Whitespace is ignored, commands are ended by a `;`
* Arguments are held within brackets and are named, bound by `=`
* Arguments that have default values do not need to be specified
* Arguments are separated by `,`
* Return values are printed to the error stream (same as logging), but prefixed with `<Timestamp>[DBGR]: `

#### Argument types
* `uint`
Unsigned integer.  Base-10 unless prefixed with `0x` which means base-16, or `0` which means octal
* `ternary`
10-trit unsigned ternary.  Base-10 input unless prefixed with `t` which means base-3
* `reg`
vCPU register, `A`, `C`, or `D` (case-sensitive)
* `string`
ACSII string, must be surrounded by `""` and standard escaping rules apply.  Hex and octal escaping supported too

#### Functions
##### add_breakpoint
Adds a breakpoint at the given address.  If one already exists at `address` then the previous one is replaced.  If there are any breakpoints listed in a script, then at least one must appear before the ` run` command.
###### Arguments:
`address = uint/ternary [Required]`
Memory location in the virtual memory block to attach the breakpoint
`ignore_count [Defaults to 0]`
Number of times `address` is hit before execution is paused
```
add_breakpoint(address=9, ignore_count=2);
```
---

##### remove_breakpoint
Removes a breakpoint at the given address.  If no breakpoint exists at `address` then it is a no-op.
###### Arguments:
`address = uint/ternary [Required]`
Memory location in the virtual memory block to remove a breakpoint from
```
remove_breakpoint(address=9);
```
---

##### run
Begins execution of the program.  If `max_runtime_ms` is non-zero then program execution is stopped (_stopped_, not paused) after the number of milliseconds requested unless a breakpoint is hit first.  Subsequent commands in the script are not read until a breakpoint is hit or the program ends.

There can only be a single `run` command in a script, use `resume` to continue execution after a breakpoint is hit.

###### Arguments:
`max_runtime_ms = uint/ternary [Defaults to 0, which represents infinite runtime]` 
Number of milliseconds the program is allowed to run without hitting a breakpoint.
```
run();
```
---

##### address_value
Returns the value at the given address.  Although not a requirement, it is generally only meaningful to call this function when the program is paused.
###### Arguments:
`address = uint/ternary [Required]`
Memory location in the virtual memory block to read.
```
address_value(address=9);
```
Example output:
```
2020-12-19 11:40:58.696620673[DBGR]: address_value(address={d:9, t:0000000100}) = {d:125, t:0000011122}
```
---

##### register_value
Returns the value at the given register.  Although not a requirement, it is generally only meaningful to call this function when the program is paused.

If the register is C or D, then the register contains an address - this is printed along with the value at the address in virtual memory.
###### Arguments:
`reg = reg [Required]`
Memory location in the virtual memory block to read.
```
register_value(reg=A);
```
Example output:
```
2020-12-19 11:40:58.696758245[DBGR]: register_value(reg=C) = {{d:9, t:0000000100}, {d:125, t:0000011122}}
```
---

##### step
In a paused state, this will advance program execution by a single instruction.  Script parsing will fail if this function is before a `run`.

```
step();
```
---

##### resume
In a paused state, this will resume program execution.  Script parsing will fail if this function is before a `run`.

```
resume();
```
---

##### stop
In a paused state, this will stop program execution.  If present in a script, it must be last.

```
stop();
```
---

##### on_input
Adds a string onto the input data queue.  When the program requests input data (i.e. from `cin`), then the next string in this queue is passed to it or the program will block waiting for input.  This allows you to 'pre-load' input data before starting program execution.
###### Arguments:
`data = string [Required]`
Program input.
```
on_input(data="Hello world!");
```
---

### Examples
This script is used inside the unit tests on the `hello_world` program.
```
add_breakpoint(address=9);
add_breakpoint(address=19);
run();
address_value(address=9);
register_value(reg=A);
register_value(reg=C);
register_value(reg=D);

step();
address_value(address=10);
register_value(reg=A);
register_value(reg=C);
register_value(reg=D);

resume();
address_value(address=19);
register_value(reg=A);
register_value(reg=C);
register_value(reg=D);

add_breakpoint(address=20);
remove_breakpoint(address=20);
resume();
address_value(address=20);
```
This script is used inside the unit tests on the `echo` program.
```
on_input(data="Hello!");
on_input(data="Goodbye!");
run(max_runtime_ms=100);
```

## API Documentation
API documentation for Malbolge is available [here](https://cmannett85.github.io/malbolge).

## Dependencies
* C++ toolchain supporting C++20 (only tested using g++ v10.2)
* Boost v1.67
* CMake v3.12
* Emscripten v2.0.10 (only needed for WASM build)
* Doxygen (only needed for Documentation build)

---

## Playground
You can try out Malbolge programs in your browser using the [Playground](https://cmannett85.github.io/malbolge/playground).  There are a couple of built in presets to get you started.

It also has features that allow you to convert between normalised and denormalised Malbolge.  Normalisation is Malbolge but with the whitespace and [initial mapping](#pre-ciphering) removed leaving only the initial [vCPU instructions](#vcpu-instructions), denormalisation is the reverse.

---

## What is Malbolge?
Malbolge is the name of the eighth circle of Hell in Dante's Inferno.  This sets the scene.

Invented in 1998 by Ben Olmstead, Malbolge isn't so much a programming language for humans, but a machine language for a fictional 10-trit ternary-based virtual CPU with 3 registers and a fixed size memory block.

#### Trit?  Ternary?

A trit is single unit of state in the Malbolge VM, it holds 1 of 3 possible states: `0`, `1`, or `2`.  In other words it is the 3-state equivalent of a bit.  A ternary is simply an unsigned integer made from trits.

Malbolge has a single native type, a 10-trit ternary (hereby referred to as just 'ternary').  3<sup>10</sup> gives you 59049 possible values, in the range [0, 59048].  The values wrap upon under- and overflow.

#### Fixed Size Memory Block
The Malbolge virtual CPU is accompanied by a fixed size block of memory that holds 59049 addressable cells with each big enough to hold a single 10-trit ternary.

The fact that the virtual memory's size is fixed by the language itself, is why Malbolge cannot be considered Turing-complete.

<a name="vcpu-registers"></a>
#### vCPU Registers
The virtual CPU has 3 ternary registers:
 - `a`: 'Accumulator', a general-purpose register
 - `c`: 'Code pointer', holds the memory location of the current or next instruction
 - `d`: 'Data pointer', holds the memory location of the current or next data value

<a name="pre-ciphering"></a>
#### Pre-ciphering
When a CPU cycle is begun, the value at the code pointer needs to undergo a simple ciphering in order to extract the actual value:
```
i = (*c - 33 + c) % 94
```
The subtraction of 33 is why `*c` must be checked to see if it lies in the graphical ASCII range ([33, 126]) first, if it doesn't it is an error and the program must abort.  `i` is an index into the pre-cipher table:
```
+b(29e*j1VMEKLyC})8&m#~W>qxdRp0wkrUo[D7,XTcA"lI.v%{gJh4G\-=O@5`_3i<?Z';FNQuY]szf$!BS/|t:Pn6^Ha
```

<a name="post-ciphering"></a>
#### Post-ciphering
Once processing on a CPU instruction has finished, it needs to be ciphered again before being written back into `*c` - that's right, Malbolbge modifies its code as it executes.  What fun.
```
i = *c - 33
```
Much like the [pre-ciphering](#pre-ciphering), the subtraction of 33 is why `*c` must be checked to see if it lies in the graphical ASCII range ([33, 126]) first, if it doesn't it is an error and the program must abort.  `i` is an index into the post-cipher table:
```
5z]&gqtyfr$(we4{WP)H-Zn,[%\3dL+Q;>U!pJS72FhOA1CB6v^=I_0/8|jsb9m<.TVac`uY*MK'X~xDl}REokN:#?G"i@
```

#### Ternary Op
Another unique 'feature' of Malbolge is the operation performed on two ternary values to produce another.  It is used for obfuscating the initial program data size in a memory block and can be triggered by a particular CPU instruction.

Each trit in the two input ternarys is used as an index into a table to produce a new trit value:

<table>
  <tr>
    <td colspan="2" rowspan="2"></td>
    <th colspan="3" style="text-align:center">a</td>
  </tr>
  <tr>
    <th>0</td>
    <th>1</td>
    <th>2</td>
  </tr>
  <tr>
    <th style="text-align:center" rowspan="3">b</td>
    <th>0</td>
    <td>1</td>
    <td>0</td>
    <td>0</td>
  </tr>
  <tr>
    <th>1</td>
    <td>1</td>
    <td>0</td>
    <td>2</td>
  </tr>
  <tr>
    <th>2</td>
    <td>2</td>
    <td>2</td>
    <td>1</td>
  </tr>
</table>

For example:
```
a = 0001112220
b = 0120120120
    1120020211
```

**Note** this is referred to as the 'crazy operation' on Wikipedia, I don't know where that name came from as it is not in the original specification or reference implementation - so I don't use it.  Also,
 it's not crazy, it's ridiculous.

<a name="vcpu-instructions"></a>
#### vCPU Instructions
| Instruction | (as decimal) |                                                            Execution                                                            |
|:-----------:|:-----------:|:--------------------------------------------------------------------------------------------------------------------------------|
|      j      |     106     | Sets `d` to `*d`                                                                                                                |
|      i      |     105     | Sets `c` to `*d`                                                                                                                |
|      *      |      42     | Rotates the trits in `*d` to the right i.e. towards the least significant, and copy into `a`                                    |
|      p      |     112     | Perform the op using `a` and `*d`, then assign to `*d` and `a`                                                                  |
|      /      |      47     | Take a char from stdin and assign to `a`.  `EOF` is 59048                                                                       |
|      <      |      60     | Write `a` to stdout.  Skip if `a` is 59048                                                                                      |
|      v      |     118     | Stops execution and exits the program                                                                                           |
|      o      |     111     | No-op.  This is only useful during program load, as all graphical ASCII non-instruction characters are ignored during execution |

### Program Load
On start the program data is processed:
 1. If a character is whitespace, it is discarded
 2. If a character is not graphical ASCII (i.e. outside the range [33, 126]), the program is malformed and execution aborted
 3. The character is [_pre-ciphered_](#pre-ciphering), and if the result is not an instruction, the program is malformed and execution aborted
 4. The program data, without whitespace and using the original instructions (not the pre-ciphered output), is then loaded into the start of the memory
 5. The remaining memory cells are then transformed using the result of the ternary op on the previous two cells i.e. `m[i] = op(m[i-1], m[i-2])`

### Program Execution
At program start, all the [registers](#vcpu-registers) are set to zero. Then the program follows this loop:
 1. Take the value at `*c` and [pre-cipher](#pre-ciphering) it.  If the input is not in the graphical ASCII range then the program is malformed and must abort
 2. If the result is an instruction then execute it, otherwise do nothing
 3. [Post-cipher](#post-ciphering) the result, and write it back to `*c`
 4. Increment `c` and `d`, they will wrap upon overflow

## Malbolge Reference
It should be noted that the specification and implementation differ (of course!), most notably in that the input and output instructions are reversed.  As most of the existing implementations are based on the original C code, and therefore most of the existing Malbolge programs are written for that, I too have followed the reference implementation.

#### Specification
http://www.lscheffer.com/malbolge_spec.html

#### Reference implementation
http://www.lscheffer.com/malbolge_interp.html

---

## CI System
The Malbolge project uses Github Actions to enfore strict code quality checks during a pull request:
- Unit tests are ran
- Unit test code coverage calculated and if it is >1% lower than the previous value, the PR is rejected (can be manually overridden)
- Executable ran with a Hello World program and then checked with:
  - AddressSanitizer
  - LeakSanitizer
  - UndefinedBehaviorSanitizer
  - ThreadSanitizer
- Documentation generated, if it fails (because the API has changed and the documentation comments not updated to match), then the PR is rejected
- Release debian package and archives generated.  The debian package is installed, malbolge ran, and then uninstalled - any failure in that process will cause the PR to be rejected
- A WASM build is performed

I dabbled in `clang-format`, but it only seemed to make the formatting worse, so that will have to be manually checked.  `clang-tidy` will be integrated once it supports C++20 ranges, currently v10 produces errors for them that cannot be suppressed.

Once the PR is merged, the API documentation is generated and pushed to the [public documentation](https://cmannett85.github.io/malbolge) site.

## Contributing
Issues and PRs are always welcome, but seriously - why are you using it?

### Can I integrate the source into my own project?
Sure, but...  Err... **Why do you need it!?**
