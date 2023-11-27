import {readFileSync} from 'fs';

// Type constants.
const None = 0;

// Map from type names to type constants.
const types = { None };

let text = readFileSync("c.txt", "utf8");
let lines = text.split('\n');
let rules = gatherRules(lines);
let states = getStates(rules);
// for (let r of rules) console.log(showRule(r));
// sortRules(states);
handleSubRanges(states);
handleMidRanges(states);
collapseRanges(states);
sortRules(states);
for (let name in states) console.log(showState(states[name]));
console.log('\n' < '>');

//rules = rules.map(expand).flat();
//rules = rules.map(unescape);
//let states = getStates(rules);
//checkTargets(states, rules);
//sortStates(states);
//console.log(states);

// ---------- Rules ------------------------------------------------------------
// A rule is an object { line, base, look, string, target, type } which is a
// unit of information from a language description. It has a line number, a
// base state name, a lookahead flag, a single unescaped pattern string, a
// target state name, and a type.

// Display a rule in original format. Re-escape, and show None as blank. A
// string \n..n or \s..s is a newline or space as a range so it can be
// overridden by explicit \n or \s rules.
function showRule(rule) {
    let s = rule.string, a = rule.look;
    if (s[0] == ' ') s = "s" + s.substring(1);
    if (s[0] == '\n') s = "n" + s.substring(1);
    if (s[3] == ' ') s = s.substring(0,3) + "s";
    if (s[3] == '\n') s = s.substring(0,3) + "n";
    let p = "";
    if (a) p = "\\";
    if (s[0] == "\\") p += "\\" + s;
    else p += s;
    let out = rule.base.padEnd(9, ' ');
    out += " " + p.padEnd(9, ' ');
    out += " " + rule.target.padEnd(9, ' ');
    if (rule.type != "None") out += rule.type;
    return out;
}

// Gather the rules from the lines of the file.
function gatherRules(lines) {
    let rules = [];
    for (let i = 0; i < lines.length; i++) {
        let line = lines[i].trim();
        if (line.length == 0) continue;
        if ('a' <= line[0] && line[0] <= 'z') {
            let strings = line.split(/ +/);
            rules = rules.concat(makeRules(i+1, strings));
        }
    }
    return rules;
}

// Create rules from a line of strings. Expand \ to ranges \n..n \s..s \!..~
function makeRules(line, strings) {
    let rules = [];
    if (strings.length < 3) throw new Error("incomplete rule on line " + row);
    let base = strings[0];
    strings.shift();
    let type = strings[strings.length - 1];
    if ('A' <= type[0] && type[0] <= 'Z') {
        strings.pop();
        if (strings.length < 2) {
            throw new Error("incomplete rule on line " + row);
        }
    }
    else type = "None";
    let target = strings.pop();
    if (! ('a' <= target[0] && target[0] <= 'z')) {
        throw new Error("expecting target state on line " + row);
    }
    let strings2 = [];
    for (let s of strings) {
        if (s == "\\") {
            strings2 = strings2.concat(["\\\n..\n", "\\ .. ", "\\!..~"]);
        }
        else strings2.push(s);
    }
    for (let s of strings2) {
        let [look, string] = unescape(s);
        let rule = { line, base, look, string, target, type };
        rules.push(rule);
    }
    return rules;
}

// Deal with backslashes, returning a lookahead flag and plain string:
//   \\\... -> \... look
//   \\...  -> \...
//   \s     -> SP look
//   \n     -> NL look
//   \x     -> error
//   \...   -> ... look
function unescape(string) {
    if (string.startsWith("\\\\\\")) return [true, string.substring(2)];
    else if (string.startsWith("\\\\")) return [false, string.substring(1)];
    else if (string === "\\s") return [true, " "];
    else if (string === "\\n") return [true, "\n"];
    else if (string.startsWith("\\") && string.length == 2) {
        throw new Error("bad lookahead " + string + " on line " + row);
    }
    else if (string.startsWith("\\")) return [true, string.substring(1)];
    else return [false, string];
}

// ---------- States -----------------------------------------------------------
// A state is an object { row, name, rules } with a row in the transition table,
// and the relevant rules. The row is 0 for the first state mentioned in the
// language description.

function showState(state) {
    let out = "";
    for (let r of state.rules) out += showRule(r) + "\n";
    return out;
}

// Collect the states from the rules as a map from state name to state.
// Check that all the targets exist.
function getStates(rules) {
    let row = 0;
    let states = {};
    for (let rule of rules) {
        let {base} = rule;
        let state = states[base];
        if (state == undefined) {
            state = { row, name:base, rules:[] };
            states[base] = state;
            row++;
        }
        state.rules.push(rule);
    }
    for (let rule of rules) {
        let {target} = rule;
        if (states[target] != undefined) continue;
        throw new Error("unknown target " + target + " on line " + rule.row);
    }
    return states;
}

// ---------- Ranges -----------------------------------------------------------
// Ranges are reduced to deal with allowed ambiguities. First, ranges are
// compared with each other. If one is a subrange of the other, e.g. 2..8 and
// and 0..9, then the longer one is split and the overlap discarded, e.g. 0..9
// becomes 0..1 and 9..9. A one-character range like 9..9 is retained, in case
// it is overridden by a non-range. Then ranges are compared with one-character
// patterns, e.g. 5 causes 0..9 to be split into 0..4 and 6..9. Finally,
// one character ranges are collapsed, e.g. 9..9 becomes 9.

function isRange(s) {
    if (s.length != 4) return false;
    if (s[1] != '.' || s[2] != '.') return false;
    return true;
}

function subRange(s, t) {
    return s[0] >= t[0] && s[3] <= t[3];
}

function overlap(s, t) {
    if (s[0] < t[0] && s[3] >= t[0] && s[3] < t[3]) return true;
    if (t[0] < s[0] && t[3] >= s[0] && t[3] < s[3]) return true;
    return false;
}

// Look for a pair of ranges and split one. Return whether a change was made.
function handleSubRange(rules) {
    let changed = false;
    for (let i = 0; i < rules.length; i++) {
        let s = rules[i].string;
        if (! isRange(s)) continue;
        let sn = s[3] - s[0];
        for (let j = 0; j < rules.length; j++) {
            if (j == i) continue;
            let t = rules[j].string;
            if (! isRange(t)) continue;
            let tn = t[3] - t[0];
            if (tn < sn) continue;
            if (overlap(s, t)) {
                throw new Error(
                    "overlapping ranges " + s + " and " + t +
                    " on lines " + rules[i].line + " and " + rules[j].line
                );
            }
            if (subRange(s,t)) {
                if (s[0] == t[0]) {
                    let ch = String.fromCharCode(s[3].charCodeAt()+1);
                console.log(s, t, rules[j].string,"to",ch + ".." + t[3]);
                    rules[j].string = ch + ".." + t[3];
                }
                else if (s[3] == t[3]) {
                    let ch = String.fromCharCode(s[0].charCodeAt()-1);
                    rules[j].string = t[0] + ".." + ch;
                }
                else {
                    let c1 = String.fromCharCode(s[0].charCodeAt()-1);
                    let c2 = String.fromCharCode(s[3].charCodeAt()+1);
                    rules[j].string = t[0] + ".." + c1;
                    let string = c2 + ".." + t[3];
                    let { line, base, look, target, type } = rules[j];
                    rules.push({ line, base, look, string, target, type });
                }
                changed = true;
            }
        }
    }
    return changed;
}

// Look for a singleton within a range, e.g 5 in 0..9. Split the range, e.g.
// into 0..4 and 6..9. Return whether a change was made.
function handleMidRange(rules) {
    let changed = false;
    for (let i = 0; i < rules.length; i++) {
        let s = rules[i].string;
        if (s.length != 1) continue;
        for (let j = 0; j < rules.length; j++) {
            let t = rules[j].string;
            if (! isRange(t)) continue;
            if (! (t[0] <= s[0] && s[0] <= t[3])) continue;
            if (s[0] == t[0] && s[0] == t[3]) {
                rules.splice(j,1);
            }
            else if (s[0] == t[0]) {
                let ch = String.fromCharCode(t[0].charCodeAt()+1);
                rules[j].string = ch + ".." + t[3];
            }
            else if (s[0] == t[3]) {
                let ch = String.fromCharCode(t[3].charCodeAt()-1);
                rules[j].string = t[0] + ".." + ch;
            }
            else {
                let c1 = String.fromCharCode(s[0].charCodeAt()-1);
                let c2 = String.fromCharCode(s[0].charCodeAt()+1);
                rules[j].string = t[0] + ".." + c1;
                let string = c2 + ".." + t[3];
                let { line, base, look, target, type } = rules[j];
                rules.push({ line, base, look, string, target, type });
            }
            changed = true;
            break;
        }
    }
    return changed;
}

function handleSubRanges(states) {
    for (let s in states) {
        let state = states[s];
        let changed = true;
        while (changed) changed = handleSubRange(state.rules);
    }
}
function handleMidRanges(states) {
    for (let s in states) {
        let state = states[s];
        let changed = true;
        while (changed) changed = handleMidRange(state.rules);
    }
}
function collapseRanges(states) {
    for (let s in states) {
        let state = states[s];
        let rules = state.rules;
        for (let rule of rules) {
            let s = rule.string;
            if (! isRange(s)) continue;
            if (s[0] != s[3]) continue;
            rule.string = s[0];
        }
    }
}

// ----------

// Compare two rules in a state by pattern string.
function compareRules(r1, r2) {
    if (r1.string == r2.string) return 0;
    if (r1.string < r2.string) return -1;
    return 1;
}

// Sort the rules in each state.
function sortRules(states) {
    for (let s in states) states[s].rules.sort(compareRules);
}
