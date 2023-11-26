import {readFileSync} from 'fs';

// Types
const None = 0;


let text = readFileSync("c.txt", "utf8");
let lines = text.split('\n');
let rules = getRules(lines);
rules = rules.map(expand).flat();
rules = rules.map(unescape);
let states = getStates(rules);
checkTargets(states, rules);
sortStates(states);
console.log(states);

// Extract the rules from the lines.
function getRules(lines) {
    let rules = [];
    for (let i = 0; i < lines.length; i++) {
        let line = lines[i].trim();
        if (line.length == 0) continue;
        if ('a' <= line[0] && line[0] <= 'z') {
            let strings = line.split(/ +/);
            rules.push(makeRule(i+1, strings));
        }
    }
    return rules;
}

// Create a rule from a line of strings.
function makeRule(row, strings) {
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
    else type = "";
    let target = strings.pop();
    if (! ('a' <= target[0] && target[0] <= 'z')) {
        throw new Error("expecting target state on line " + row);
    }
    return { row, base, strings, target, type };
}

// Expand a rule into an array of mini-rules with one pattern each.
function expand(rule) {
    let {row, base, strings, target, type} = rule;
    let rules = [];
    for (let string of strings) {
        rules.push({row, base, string, target, type});
    }
    return rules;
}

// Deal with backslashes:
//   \\\... -> \... look
//   \\...  -> \...
//   \      -> NL..~
//   \s     -> SP look
//   \n     -> NL look
//   \x     -> error
//   \...   -> ... look
function unescape(rule) {
    let {row, base, string, target, type} = rule;
    if (string.startsWith("\\\\\\")) {
        rule.look = true;
        rule.string = string.substring(2);
    }
    else if (string.startsWith("\\\\")) {
        rule.string = string.substring(1);
    }
    else if (string === "\\") {
        rule.look = true;
        rule.string = "\n..~";
    }
    else if (string === "\\s") {
        rule.look = true;
        rule.string = " ";
    }
    else if (string === "\\n") {
        rule.look = true;
        rule.string = "\n";
    }
    else if (string.startsWith("\\") && string.length == 2) {
        throw new Error("bad lookahead " + string + " on line " + row);
    }
    else if (string.startsWith("\\")) {
        rule.look = true;
        rule.string = string.substring(1);
    }
    return rule;
}

// Collect the rules into states.
function getStates(rules) {
    let states = {};
    for (let rule of rules) {
        let {base} = rule;
        if (states[base] == undefined) states[base] = [];
        states[base].push(rule);
    }
    return states;
}

// Check that all the targets exist.
function checkTargets(states, rules) {
    for (let rule of rules) {
        let {target} = rule;
        if (states[target] != undefined) continue;
        throw new Error("unknown target " + target + " on line " + row);
    }
}

function sortStates(states) {
    for (let name in states) {
        states[name].sort(compareRules);
    }
}

function compareRules(r1, r2) {
    return r1.string.localeCompare(r2.string);
}
