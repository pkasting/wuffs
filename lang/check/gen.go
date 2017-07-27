// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// +build ignore

package main

// gen.go generates data.go.
//
// Invoke it via "go generate".

import (
	"bytes"
	"fmt"
	"go/format"
	"io/ioutil"
	"os"
	"strings"
)

var reasons = [...]string{
	"a < (b + c): a < c; 0 <= b",
	"(a + b) <= c: a <= (c - b)",
	"a < b: a < c; c <= b",
	"a <= b: a <= c; c <= b",
	"a < (b + c): a < (b0 + c0); b0 <= b; c0 <= c",
}

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	out.WriteString(header)
	for _, reason := range reasons {
		nextTmp = 0
		names = map[string]bool{}
		if err := gen(reason); err != nil {
			return err
		}
	}
	out.WriteString(footer)

	formatted, err := format.Source(out.Bytes())
	if err != nil {
		return err
	}
	return ioutil.WriteFile("data.go", formatted, 0644)
}

var (
	nextTmp int
	names   map[string]bool
	out     bytes.Buffer
)

func gen(reason string) error {
	fmt.Fprintf(&out, "\n{`%q`, func(q *checker, n *a.Assert) error {\n", reason)

	for i := 0; i < len(reason); i++ {
		b := reason[i]
		if (b < 0x20) || (0x7F <= b) {
			return fmt.Errorf("bad reason %q", reason)
		}
	}
	i := strings.IndexByte(reason, ':')
	if i < 0 {
		return fmt.Errorf("bad reason %q", reason)
	}
	claim := strings.TrimSpace(reason[:i])
	requirements := strings.Split(reason[i+1:], ";")

	if err := genClaim(claim); err != nil {
		return fmt.Errorf("bad claim %q: %v", claim, err)
	}
	for _, req := range requirements {
		req = strings.TrimSpace(req)
		if err := genReq(req); err != nil {
			return fmt.Errorf("bad req %q: %v", req, err)
		}
	}

	fmt.Fprintf(&out, "return nil }},\n")
	return nil
}

func genClaim(claim string) error {
	n, err := parse(claim)
	if err != nil {
		return err
	}
	if err := genParseBinaryOps(n, "n.Condition()"); err != nil {
		return err
	}
	return nil
}

func genParseBinaryOps(n *node, arg string) error {
	l := genName(n.lhs)
	r := genName(n.rhs)
	key := keys[n.op]
	if key == "" {
		return fmt.Errorf("bad op %q", n.op)
	}
	fmt.Fprintf(&out, "op, %s, %s := parseBinaryOp(%s)\n", l, r, arg)
	fmt.Fprintf(&out, "if op.Key() != t.Key%s { return errFailed }\n", key)
	if l[0] == 't' {
		if err := genParseBinaryOps(n.lhs, l); err != nil {
			return err
		}
	}
	if r[0] == 't' {
		if err := genParseBinaryOps(n.rhs, r); err != nil {
			return err
		}
	}
	return nil
}

func genName(n *node) string {
	if isConstant(n.op) {
		if n.op == "0" {
			return "zeroExpr"
		}
		return "$BAD_CONSTANT"
	}
	if isVariable(n.op) {
		names[n.op] = true
		return "x" + n.op
	}
	nextTmp++
	return fmt.Sprintf("t%d", nextTmp-1)
}

func genReq(req string) error {
	n, err := parse(req)
	if err != nil {
		return err
	}
	fmt.Fprintf(&out, "// %s\n", req)
	genArgValues(n)
	return genProveReasonRequirement(n, "")
}

func genArgValues(n *node) {
	switch {
	case isVariable(n.op):
		if names[n.op] {
			break
		}
		names[n.op] = true
		fmt.Fprintf(&out, "x%s := argValue(q.tm, n.Args(), %q)\n", n.op, n.op)
		fmt.Fprintf(&out, "if x%s == nil { return errFailed }\n", n.op)
	case isOp(n.op):
		genArgValues(n.lhs)
		genArgValues(n.rhs)
	}
}

func genProveReasonRequirement(n *node, name string) error {
	l := genName(n.lhs)
	r := genName(n.rhs)
	key := keys[n.op]
	if key == "" {
		return fmt.Errorf("bad op %q", n.op)
	}
	if l[0] == 't' {
		if err := genProveReasonRequirement(n.lhs, l); err != nil {
			return err
		}
	}
	if r[0] == 't' {
		if err := genProveReasonRequirement(n.rhs, r); err != nil {
			return err
		}
	}
	if name == "" {
		fmt.Fprintf(&out, "if err := proveReasonRequirement(q, t.ID%s, %s, %s); err != nil { return err }\n",
			key, l, r)
	} else {
		fmt.Fprintf(&out, "%s := a.NewExpr(a.FlagsTypeChecked, t.ID%s, 0, %s.Node(), nil, %s.Node(), nil)\n",
			name, key, l, r)
	}
	return nil
}

func parse(s string) (*node, error) {
	n, s, err := parseExpr(s)
	if err != nil {
		return nil, err
	}
	if s != "" {
		return nil, fmt.Errorf("parse error")
	}
	return n, nil
}

func parseExpr(s string) (*node, string, error) {
	lhs, s, err := parseOperand(s)
	if err != nil {
		return nil, "", err
	}
	s = trim(s)
	if !isOp(s) {
		return nil, "", fmt.Errorf("parseExpr error")
	}
	i := 1
	for ; i < len(s) && isOpByte(s[i]); i++ {
	}
	op, s := s[:i], s[i:]
	s = trim(s)
	rhs, s, err := parseOperand(s)
	if err != nil {
		return nil, "", err
	}
	s = trim(s)
	return &node{
		op:  op,
		lhs: lhs,
		rhs: rhs,
	}, s, nil
}

func parseOperand(s string) (*node, string, error) {
	switch {
	case isConstant(s), isVariable(s):
		i := 1
		for ; i < len(s) && isAlphaNumByte(s[i]); i++ {
		}
		return &node{op: s[:i]}, s[i:], nil
	case isParen(s):
		n, s, err := parseExpr(s[1:])
		if err != nil {
			return nil, "", err
		}
		if len(s) > 0 && s[0] == ')' {
			return n, s[1:], nil
		}
	}
	return nil, "", fmt.Errorf("parseOperand error")
}

type node struct {
	op  string
	lhs *node
	rhs *node
}

func (n *node) String() string {
	if n == nil {
		return "$NIL_NODE"
	}
	switch {
	case isConstant(n.op), isVariable(n.op):
		return n.op
	case isOp(n.op):
		return fmt.Sprintf("(%s %v %v)", n.op, n.lhs, n.rhs)
	}
	return "$BAD_NODE"
}

func isConstant(s string) bool { return s != "" && '0' <= s[0] && s[0] <= '9' }
func isOp(s string) bool       { return s != "" && isOpByte(s[0]) }
func isParen(s string) bool    { return s != "" && '(' == s[0] }
func isVariable(s string) bool { return s != "" && 'a' <= s[0] && s[0] <= 'z' }

func isOpByte(b byte) bool       { return ('<' <= b && b <= '>') || '+' == b || '-' == b || '!' == b }
func isAlphaNumByte(b byte) bool { return ('0' <= b && b <= '9') || ('a' <= b && b <= 'z') }

func trim(s string) string {
	for ; len(s) > 0 && s[0] == ' '; s = s[1:] {
	}
	return s
}

var keys = map[string]string{
	"+":  "XBinaryPlus",
	"-":  "XBinaryMinus",
	"!=": "XBinaryNotEq",
	"<":  "XBinaryLessThan",
	"<=": "XBinaryLessEq",
	"==": "XBinaryEqEq",
	">=": "XBinaryGreaterEq",
	">":  "XBinaryGreaterThan",
}

const header = `// Code generated by running "go generate". DO NOT EDIT.

// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

var reasons = [...]struct {
	s string
	r reason
}{
`

const footer = "}\n"
