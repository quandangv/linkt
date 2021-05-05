# linkt
Convert configuration files into dynamic tree structures

![Codecov](https://img.shields.io/codecov/c/gh/quandangv/linkt?style=for-the-badge)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/quandangv/linkt?style=for-the-badge)
![GitHub Workflow Status](https://img.shields.io/github/workflow/status/quandangv/linkt/CI?style=for-the-badge)
![GitHub](https://img.shields.io/github/license/quandangv/linkt?style=for-the-badge)
![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/quandangv/linkt?style=for-the-badge)

## Building the project
Run `./build.sh -A` in the project root to build and install the project with default settings. To view other build options, run `./build.sh -h`.

To use Linkt as a C++ library, you may need to set the prefix for installation as `/usr` instead of `/usr/lib`. Do this by adding the option `--prefix /usr`.

## Usage
Examples of usages can be found in the directory `test/examples`

### Linkt_replace
**Syntax** `linkt_replace [-i tree-file]... [input-file output-file]`

Searches input-file for escaped expression in the form of `${expr}` and replace them with the value of `expr`.
Escaped expressions may also take the form of `${expr ? fallback}`, if so, `fallback` will be returned if `expr` produces an exception. Some types of expression returns fallback in different conditions, which are documented below.

Options:
* `-i tree-file` - parse `tree-file` to get the data tree that will help with the replacement.

### Expression types
Here is a list of expressions type, their values, and the condition for fallback to be returned:
* `ref-path` - the value of the node at `ref-path` of the data tree
  * Fallback is returned if the specified node doesn't exist.
  * Note that this is the only expression type that contains 1 components. Other expression types have more than 1.
* `cmd <bash-cmd>` - the output of `bash-cmd`
  * Fallback is returned if an exception occourred or `bash-cmd` returns a non-zero exit code
* `env <var-name>` - the value of the environment variable VAR-NAME.
  * Fallback is returned if the variable is not set.
* `file <file-name>` - The content of the specified file.
  * Fallback is returned if the file fails to be opened.
* `color [colorspace] [component-action] <source-color>` - Convert `source-color` to RGB hexadecimal color code
  * If `component-action` is specified, first convert to `colorspace` (default is CIELab), apply the component actions, then convert to RGB hexadecimal.
    * The component actions takes the form of `component ? amount`
      * `component` depends on the colorspace used. For example: the available components for CIELab are L, a, b; for HSV are H, S, and V.
      * `?` may be one of the operators `+`, `-`, `*`, `/`, `=`, representing the corresponding operation to the component.
      * `amount` is the amount applied using the operator
    * Available colorspaces are RGB, HSV, HSL, CIELab, CIELch, Jzazbz, and JzCzhz
* `poll <poll-cmd>` - the command is executed once at the first call to the node
  * If the command prints to its output some time between a call and its previous call, returns the text of the last line of output. Otherwise, return the fallback
* `map <from-range> <range2> <value>` - Linearly interpolate `value` from `from-range` to `to-range`
  * Ranges may take the form of either `from:to` or `to`. If `from` is omitted, the default of 0 will be used

The arguments of the commands above are separated by spaces, unless that space is enclosed by quotes, brackets, or parenthesis.

Matching starting and ending quotes are removed. To prevent text from being separated into multiple components, enclose it in quotes. Single and double quotes can be used interchangeably
