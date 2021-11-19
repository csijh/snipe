// Snipe language compiler. Free and open source. See licence.txt.
#include "rules.h"

// A list of patterns. The patterns from all the rules are collected into a
// single list. The patterns are normalised as they are collected, so that they
// are unique and atomic. An atomic pattern is either a single string, or a
// range of characters, each of which starts with a high UTF-8 byte (>= 128). A
// range of low-byte characters such as 0..9 is expanded into individual
// one-byte strings. A range-of high-byte characters is divided into separate
// ranges, each representing characters starting with the same byte. In
// addition, if two ranges overlap, they are replaced by three non-overlapping
// ranges. For example, \200..\400 and \300..\500 are replaced by \200..\299 and
// \300..\399 and \400..\500.
