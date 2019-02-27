/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
//
// Copyright (C) 1991 Texas Instruments Incorporated.
//
// Permission is granted to any individual or institution to use, copy, modify
// and distribute this software, provided that this complete copyright and
// permission notice is maintained, intact, in all copies and supporting
// documentation.
//
// Texas Instruments Incorporated provides this software "as is" without
// express or implied warranty.
//
//
// Created: MNF 06/13/89  Initial Design and Implementation
// Updated: LGO 08/09/89  Inherit from Generic
// Updated: MBN 09/07/89  Added conditional exception handling
// Updated: MBN 12/15/89  Sprinkled "const" qualifiers all over the place!
// Updated: DLS 03/22/91  New lite version
//

#include "kwsysPrivate.h"
#include KWSYS_HEADER(RegularExpression.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#  include "RegularExpression.hxx.in"
#endif

#include <stdio.h>
#include <string.h>

namespace KWSYS_NAMESPACE {

// RegularExpression -- Copies the given regular expression.
RegularExpression::RegularExpression(const RegularExpression& rxp)
{
  if (!rxp.program) {
    this->program = KWSYS_NULLPTR;
    return;
  }
  int ind;
  this->progsize = rxp.progsize;            // Copy regular expression size
  this->program = new char[this->progsize]; // Allocate storage
  for (ind = this->progsize; ind-- != 0;)   // Copy regular expression
    this->program[ind] = rxp.program[ind];
  // Copy pointers into last successful "find" operation
  this->regmatch = rxp.regmatch;
  this->regmust = rxp.regmust; // Copy field
  if (rxp.regmust != KWSYS_NULLPTR) {
    char* dum = rxp.program;
    ind = 0;
    while (dum != rxp.regmust) {
      ++dum;
      ++ind;
    }
    this->regmust = this->program + ind;
  }
  this->regstart = rxp.regstart; // Copy starting index
  this->reganch = rxp.reganch;   // Copy remaining private data
  this->regmlen = rxp.regmlen;   // Copy remaining private data
}

// operator= -- Copies the given regular expression.
RegularExpression& RegularExpression::operator=(const RegularExpression& rxp)
{
  if (this == &rxp) {
    return *this;
  }
  if (!rxp.program) {
    this->program = KWSYS_NULLPTR;
    return *this;
  }
  int ind;
  this->progsize = rxp.progsize; // Copy regular expression size
  delete[] this->program;
  this->program = new char[this->progsize]; // Allocate storage
  for (ind = this->progsize; ind-- != 0;)   // Copy regular expression
    this->program[ind] = rxp.program[ind];
  // Copy pointers into last successful "find" operation
  this->regmatch = rxp.regmatch;
  this->regmust = rxp.regmust; // Copy field
  if (rxp.regmust != KWSYS_NULLPTR) {
    char* dum = rxp.program;
    ind = 0;
    while (dum != rxp.regmust) {
      ++dum;
      ++ind;
    }
    this->regmust = this->program + ind;
  }
  this->regstart = rxp.regstart; // Copy starting index
  this->reganch = rxp.reganch;   // Copy remaining private data
  this->regmlen = rxp.regmlen;   // Copy remaining private data

  return *this;
}

// operator== -- Returns true if two regular expressions have the same
// compiled program for pattern matching.
bool RegularExpression::operator==(const RegularExpression& rxp) const
{
  if (this != &rxp) {         // Same address?
    int ind = this->progsize; // Get regular expression size
    if (ind != rxp.progsize)  // If different size regexp
      return false;           // Return failure
    while (ind-- != 0)        // Else while still characters
      if (this->program[ind] != rxp.program[ind]) // If regexp are different
        return false;                             // Return failure
  }
  return true; // Else same, return success
}

// deep_equal -- Returns true if have the same compiled regular expressions
// and the same start and end pointers.

bool RegularExpression::deep_equal(const RegularExpression& rxp) const
{
  int ind = this->progsize;                     // Get regular expression size
  if (ind != rxp.progsize)                      // If different size regexp
    return false;                               // Return failure
  while (ind-- != 0)                            // Else while still characters
    if (this->program[ind] != rxp.program[ind]) // If regexp are different
      return false;                             // Return failure
  // Else if same start/end ptrs, return true
  return (this->regmatch.start() == rxp.regmatch.start() &&
          this->regmatch.end() == rxp.regmatch.end());
}

// The remaining code in this file is derived from the regular expression code
// whose copyright statement appears below.  It has been changed to work
// with the class concepts of C++ and COOL.

/*
 * compile and find
 *
 *      Copyright (c) 1986 by University of Toronto.
 *      Written by Henry Spencer.  Not derived from licensed software.
 *
 *      Permission is granted to anyone to use this software for any
 *      purpose on any computer system, and to redistribute it freely,
 *      subject to the following restrictions:
 *
 *      1. The author is not responsible for the consequences of use of
 *              this software, no matter how awful, even if they arise
 *              from defects in it.
 *
 *      2. The origin of this software must not be misrepresented, either
 *              by explicit claim or by omission.
 *
 *      3. Altered versions must be plainly marked as such, and must not
 *              be misrepresented as being the original software.
 *
 * Beware that some of this code is subtly aware of the way operator
 * precedence is structured in regular expressions.  Serious changes in
 * regular-expression syntax might require a total rethink.
 */

/*
 * The "internal use only" fields in regexp.h are present to pass info from
 * compile to execute that permits the execute phase to run lots faster on
 * simple cases.  They are:
 *
 * regstart     char that must begin a match; '\0' if none obvious
 * reganch      is the match anchored (at beginning-of-line only)?
 * regmust      string (pointer into program) that match must include, or NULL
 * regmlen      length of regmust string
 *
 * Regstart and reganch permit very fast decisions on suitable starting points
 * for a match, cutting down the work a lot.  Regmust permits fast rejection
 * of lines that cannot possibly match.  The regmust tests are costly enough
 * that compile() supplies a regmust only if the r.e. contains something
 * potentially expensive (at present, the only such thing detected is * or +
 * at the start of the r.e., which can involve a lot of backup).  Regmlen is
 * supplied because the test in find() needs it and compile() is computing
 * it anyway.
 */

/*
 * Structure for regexp "program".  This is essentially a linear encoding
 * of a nondeterministic finite-state machine (aka syntax charts or
 * "railroad normal form" in parsing technology).  Each node is an opcode
 * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
 * all nodes except BRANCH implement concatenation; a "next" pointer with
 * a BRANCH on both ends of it is connecting two alternatives.  (Here we
 * have one of the subtle syntax dependencies:  an individual BRANCH (as
 * opposed to a collection of them) is never concatenated with anything
 * because of operator precedence.)  The operand of some types of node is
 * a literal string; for others, it is a node leading into a sub-FSM.  In
 * particular, the operand of a BRANCH node is the first node of the branch.
 * (NB this is *not* a tree structure:  the tail of the branch connects
 * to the thing following the set of BRANCHes.)  The opcodes are:
 */

// definition   number  opnd?   meaning
#define END 0   // no   End of program.
#define BOL 1   // no   Match "" at beginning of line.
#define EOL 2   // no   Match "" at end of line.
#define ANY 3   // no   Match any one character.
#define ANYOF 4 // str  Match any character in this string.
#define ANYBUT                                                                \
  5 // str  Match any character not in this
    // string.
#define BRANCH                                                                \
  6               // node Match this alternative, or the
                  // next...
#define BACK 7    // no   Match "", "next" ptr points backward.
#define EXACTLY 8 // str  Match this string.
#define NOTHING 9 // no   Match empty string.
#define STAR                                                                  \
  10 // node Match this (simple) thing 0 or more
     // times.
#define PLUS                                                                  \
  11 // node Match this (simple) thing 1 or more
     // times.
#define OPEN                                                                  \
  20 // no   Mark this point in input as start of
     // #n.
// OPEN+1 is number 1, etc.
#define CLOSE 30 // no   Analogous to OPEN.

/*
 * Opcode notes:
 *
 * BRANCH       The set of branches constituting a single choice are hooked
 *              together with their "next" pointers, since precedence prevents
 *              anything being concatenated to any individual branch.  The
 *              "next" pointer of the last BRANCH in a choice points to the
 *              thing following the whole choice.  This is also where the
 *              final "next" pointer of each individual branch points; each
 *              branch starts with the operand node of a BRANCH node.
 *
 * BACK         Normal "next" pointers all implicitly point forward; BACK
 *              exists to make loop structures possible.
 *
 * STAR,PLUS    '?', and complex '*' and '+', are implemented as circular
 *              BRANCH structures using BACK.  Simple cases (one character
 *              per match) are implemented with STAR and PLUS for speed
 *              and to minimize recursive plunges.
 *
 * OPEN,CLOSE   ...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */

#define OP(p) (*(p))
#define NEXT(p) (((*((p) + 1) & 0377) << 8) + (*((p) + 2) & 0377))
#define OPERAND(p) ((p) + 3)

const unsigned char MAGIC = 0234;
/*
 * Utility definitions.
 */

#define UCHARAT(p) (reinterpret_cast<const unsigned char*>(p))[0]

#define ISMULT(c) ((c) == '*' || (c) == '+' || (c) == '?')
#define META "^$.[()|?+*\\"

/*
 * Flags to be passed up and down.
 */
#define HASWIDTH 01 // Known never to match null string.
#define SIMPLE 02   // Simple enough to be STAR/PLUS operand.
#define SPSTART 04  // Starts with * or +.
#define WORST 0     // Worst case.

/////////////////////////////////////////////////////////////////////////
//
//  COMPILE AND ASSOCIATED FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////

/*
 * Read only utility variables.
 */
static char regdummy;
static char* const regdummyptr = &regdummy;

/*
 * Utility class for RegularExpression::compile().
 */
class RegExpCompile
{
public:
  const char* regparse; // Input-scan pointer.
  int regnpar;          // () count.
  char* regcode;        // Code-emit pointer; regdummyptr = don't.
  long regsize;         // Code size.

  char* reg(int, int*);
  char* regbranch(int*);
  char* regpiece(int*);
  char* regatom(int*);
  char* regnode(char);
  void regc(char);
  void reginsert(char, char*);
  static void regtail(char*, const char*);
  static void regoptail(char*, const char*);
};

static const char* regnext(const char*);
static char* regnext(char*);

#ifdef STRCSPN
static int strcspn();
#endif

/*
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */

// compile -- compile a regular expression into internal code
// for later pattern matching.

bool RegularExpression::compile(const char* exp)
{
  const char* scan;
  const char* longest;
  size_t len;
  int flags;

  if (exp == KWSYS_NULLPTR) {
    // RAISE Error, SYM(RegularExpression), SYM(No_Expr),
    printf("RegularExpression::compile(): No expression supplied.\n");
    return false;
  }

  // First pass: determine size, legality.
  RegExpCompile comp;
  comp.regparse = exp;
  comp.regnpar = 1;
  comp.regsize = 0L;
  comp.regcode = regdummyptr;
  comp.regc(static_cast<char>(MAGIC));
  if (!comp.reg(0, &flags)) {
    printf("RegularExpression::compile(): Error in compile.\n");
    return false;
  }
  this->regmatch.clear();

  // Small enough for pointer-storage convention?
  if (comp.regsize >= 32767L) { // Probably could be 65535L.
    // RAISE Error, SYM(RegularExpression), SYM(Expr_Too_Big),
    printf("RegularExpression::compile(): Expression too big.\n");
    return false;
  }

  // Allocate space.
  //#ifndef _WIN32
  if (this->program != KWSYS_NULLPTR)
    delete[] this->program;
  //#endif
  this->program = new char[comp.regsize];
  this->progsize = static_cast<int>(comp.regsize);

  if (this->program == KWSYS_NULLPTR) {
    // RAISE Error, SYM(RegularExpression), SYM(Out_Of_Memory),
    printf("RegularExpression::compile(): Out of memory.\n");
    return false;
  }

  // Second pass: emit code.
  comp.regparse = exp;
  comp.regnpar = 1;
  comp.regcode = this->program;
  comp.regc(static_cast<char>(MAGIC));
  comp.reg(0, &flags);

  // Dig out information for optimizations.
  this->regstart = '\0'; // Worst-case defaults.
  this->reganch = 0;
  this->regmust = KWSYS_NULLPTR;
  this->regmlen = 0;
  scan = this->program + 1;       // First BRANCH.
  if (OP(regnext(scan)) == END) { // Only one top-level choice.
    scan = OPERAND(scan);

    // Starting-point info.
    if (OP(scan) == EXACTLY)
      this->regstart = *OPERAND(scan);
    else if (OP(scan) == BOL)
      this->reganch++;

    //
    // If there's something expensive in the r.e., find the longest
    // literal string that must appear and make it the regmust.  Resolve
    // ties in favor of later strings, since the regstart check works
    // with the beginning of the r.e. and avoiding duplication
    // strengthens checking.  Not a strong reason, but sufficient in the
    // absence of others.
    //
    if (flags & SPSTART) {
      longest = KWSYS_NULLPTR;
      len = 0;
      for (; scan != KWSYS_NULLPTR; scan = regnext(scan))
        if (OP(scan) == EXACTLY && strlen(OPERAND(scan)) >= len) {
          longest = OPERAND(scan);
          len = strlen(OPERAND(scan));
        }
      this->regmust = longest;
      this->regmlen = len;
    }
  }
  return true;
}

/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
char* RegExpCompile::reg(int paren, int* flagp)
{
  char* ret;
  char* br;
  char* ender;
  int parno = 0;
  int flags;

  *flagp = HASWIDTH; // Tentatively.

  // Make an OPEN node, if parenthesized.
  if (paren) {
    if (regnpar >= RegularExpressionMatch::NSUBEXP) {
      // RAISE Error, SYM(RegularExpression), SYM(Too_Many_Parens),
      printf("RegularExpression::compile(): Too many parentheses.\n");
      return KWSYS_NULLPTR;
    }
    parno = regnpar;
    regnpar++;
    ret = regnode(static_cast<char>(OPEN + parno));
  } else
    ret = KWSYS_NULLPTR;

  // Pick up the branches, linking them together.
  br = regbranch(&flags);
  if (br == KWSYS_NULLPTR)
    return (KWSYS_NULLPTR);
  if (ret != KWSYS_NULLPTR)
    regtail(ret, br); // OPEN -> first.
  else
    ret = br;
  if (!(flags & HASWIDTH))
    *flagp &= ~HASWIDTH;
  *flagp |= flags & SPSTART;
  while (*regparse == '|') {
    regparse++;
    br = regbranch(&flags);
    if (br == KWSYS_NULLPTR)
      return (KWSYS_NULLPTR);
    regtail(ret, br); // BRANCH -> BRANCH.
    if (!(flags & HASWIDTH))
      *flagp &= ~HASWIDTH;
    *flagp |= flags & SPSTART;
  }

  // Make a closing node, and hook it on the end.
  ender = regnode(static_cast<char>((paren) ? CLOSE + parno : END));
  regtail(ret, ender);

  // Hook the tails of the branches to the closing node.
  for (br = ret; br != KWSYS_NULLPTR; br = regnext(br))
    regoptail(br, ender);

  // Check for proper termination.
  if (paren && *regparse++ != ')') {
    // RAISE Error, SYM(RegularExpression), SYM(Unmatched_Parens),
    printf("RegularExpression::compile(): Unmatched parentheses.\n");
    return KWSYS_NULLPTR;
  } else if (!paren && *regparse != '\0') {
    if (*regparse == ')') {
      // RAISE Error, SYM(RegularExpression), SYM(Unmatched_Parens),
      printf("RegularExpression::compile(): Unmatched parentheses.\n");
      return KWSYS_NULLPTR;
    } else {
      // RAISE Error, SYM(RegularExpression), SYM(Internal_Error),
      printf("RegularExpression::compile(): Internal error.\n");
      return KWSYS_NULLPTR;
    }
    // NOTREACHED
  }
  return (ret);
}

/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
char* RegExpCompile::regbranch(int* flagp)
{
  char* ret;
  char* chain;
  char* latest;
  int flags;

  *flagp = WORST; // Tentatively.

  ret = regnode(BRANCH);
  chain = KWSYS_NULLPTR;
  while (*regparse != '\0' && *regparse != '|' && *regparse != ')') {
    latest = regpiece(&flags);
    if (latest == KWSYS_NULLPTR)
      return (KWSYS_NULLPTR);
    *flagp |= flags & HASWIDTH;
    if (chain == KWSYS_NULLPTR) // First piece.
      *flagp |= flags & SPSTART;
    else
      regtail(chain, latest);
    chain = latest;
  }
  if (chain == KWSYS_NULLPTR) // Loop ran zero times.
    regnode(NOTHING);

  return (ret);
}

/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
char* RegExpCompile::regpiece(int* flagp)
{
  char* ret;
  char op;
  char* next;
  int flags;

  ret = regatom(&flags);
  if (ret == KWSYS_NULLPTR)
    return (KWSYS_NULLPTR);

  op = *regparse;
  if (!ISMULT(op)) {
    *flagp = flags;
    return (ret);
  }

  if (!(flags & HASWIDTH) && op != '?') {
    // RAISE Error, SYM(RegularExpression), SYM(Empty_Operand),
    printf("RegularExpression::compile() : *+ operand could be empty.\n");
    return KWSYS_NULLPTR;
  }
  *flagp = (op != '+') ? (WORST | SPSTART) : (WORST | HASWIDTH);

  if (op == '*' && (flags & SIMPLE))
    reginsert(STAR, ret);
  else if (op == '*') {
    // Emit x* as (x&|), where & means "self".
    reginsert(BRANCH, ret);         // Either x
    regoptail(ret, regnode(BACK));  // and loop
    regoptail(ret, ret);            // back
    regtail(ret, regnode(BRANCH));  // or
    regtail(ret, regnode(NOTHING)); // null.
  } else if (op == '+' && (flags & SIMPLE))
    reginsert(PLUS, ret);
  else if (op == '+') {
    // Emit x+ as x(&|), where & means "self".
    next = regnode(BRANCH); // Either
    regtail(ret, next);
    regtail(regnode(BACK), ret);    // loop back
    regtail(next, regnode(BRANCH)); // or
    regtail(ret, regnode(NOTHING)); // null.
  } else if (op == '?') {
    // Emit x? as (x|)
    reginsert(BRANCH, ret);        // Either x
    regtail(ret, regnode(BRANCH)); // or
    next = regnode(NOTHING);       // null.
    regtail(ret, next);
    regoptail(ret, next);
  }
  regparse++;
  if (ISMULT(*regparse)) {
    // RAISE Error, SYM(RegularExpression), SYM(Nested_Operand),
    printf("RegularExpression::compile(): Nested *?+.\n");
    return KWSYS_NULLPTR;
  }
  return (ret);
}

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
char* RegExpCompile::regatom(int* flagp)
{
  char* ret;
  int flags;

  *flagp = WORST; // Tentatively.

  switch (*regparse++) {
    case '^':
      ret = regnode(BOL);
      break;
    case '$':
      ret = regnode(EOL);
      break;
    case '.':
      ret = regnode(ANY);
      *flagp |= HASWIDTH | SIMPLE;
      break;
    case '[': {
      int rxpclass;
      int rxpclassend;

      if (*regparse == '^') { // Complement of range.
        ret = regnode(ANYBUT);
        regparse++;
      } else
        ret = regnode(ANYOF);
      if (*regparse == ']' || *regparse == '-')
        regc(*regparse++);
      while (*regparse != '\0' && *regparse != ']') {
        if (*regparse == '-') {
          regparse++;
          if (*regparse == ']' || *regparse == '\0')
            regc('-');
          else {
            rxpclass = UCHARAT(regparse - 2) + 1;
            rxpclassend = UCHARAT(regparse);
            if (rxpclass > rxpclassend + 1) {
              // RAISE Error, SYM(RegularExpression), SYM(Invalid_Range),
              printf("RegularExpression::compile(): Invalid range in [].\n");
              return KWSYS_NULLPTR;
            }
            for (; rxpclass <= rxpclassend; rxpclass++)
              regc(static_cast<char>(rxpclass));
            regparse++;
          }
        } else
          regc(*regparse++);
      }
      regc('\0');
      if (*regparse != ']') {
        // RAISE Error, SYM(RegularExpression), SYM(Unmatched_Bracket),
        printf("RegularExpression::compile(): Unmatched [].\n");
        return KWSYS_NULLPTR;
      }
      regparse++;
      *flagp |= HASWIDTH | SIMPLE;
    } break;
    case '(':
      ret = reg(1, &flags);
      if (ret == KWSYS_NULLPTR)
        return (KWSYS_NULLPTR);
      *flagp |= flags & (HASWIDTH | SPSTART);
      break;
    case '\0':
    case '|':
    case ')':
      // RAISE Error, SYM(RegularExpression), SYM(Internal_Error),
      printf("RegularExpression::compile(): Internal error.\n"); // Never here
      return KWSYS_NULLPTR;
    case '?':
    case '+':
    case '*':
      // RAISE Error, SYM(RegularExpression), SYM(No_Operand),
      printf("RegularExpression::compile(): ?+* follows nothing.\n");
      return KWSYS_NULLPTR;
    case '\\':
      if (*regparse == '\0') {
        // RAISE Error, SYM(RegularExpression), SYM(Trailing_Backslash),
        printf("RegularExpression::compile(): Trailing backslash.\n");
        return KWSYS_NULLPTR;
      }
      ret = regnode(EXACTLY);
      regc(*regparse++);
      regc('\0');
      *flagp |= HASWIDTH | SIMPLE;
      break;
    default: {
      int len;
      char ender;

      regparse--;
      len = int(strcspn(regparse, META));
      if (len <= 0) {
        // RAISE Error, SYM(RegularExpression), SYM(Internal_Error),
        printf("RegularExpression::compile(): Internal error.\n");
        return KWSYS_NULLPTR;
      }
      ender = *(regparse + len);
      if (len > 1 && ISMULT(ender))
        len--; // Back off clear of ?+* operand.
      *flagp |= HASWIDTH;
      if (len == 1)
        *flagp |= SIMPLE;
      ret = regnode(EXACTLY);
      while (len > 0) {
        regc(*regparse++);
        len--;
      }
      regc('\0');
    } break;
  }
  return (ret);
}

/*
 - regnode - emit a node
   Location.
 */
char* RegExpCompile::regnode(char op)
{
  char* ret;
  char* ptr;

  ret = regcode;
  if (ret == regdummyptr) {
    regsize += 3;
    return (ret);
  }

  ptr = ret;
  *ptr++ = op;
  *ptr++ = '\0'; // Null "next" pointer.
  *ptr++ = '\0';
  regcode = ptr;

  return (ret);
}

/*
 - regc - emit (if appropriate) a byte of code
 */
void RegExpCompile::regc(char b)
{
  if (regcode != regdummyptr)
    *regcode++ = b;
  else
    regsize++;
}

/*
 - reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
void RegExpCompile::reginsert(char op, char* opnd)
{
  char* src;
  char* dst;
  char* place;

  if (regcode == regdummyptr) {
    regsize += 3;
    return;
  }

  src = regcode;
  regcode += 3;
  dst = regcode;
  while (src > opnd)
    *--dst = *--src;

  place = opnd; // Op node, where operand used to be.
  *place++ = op;
  *place++ = '\0';
  *place = '\0';
}

/*
 - regtail - set the next-pointer at the end of a node chain
 */
void RegExpCompile::regtail(char* p, const char* val)
{
  char* scan;
  char* temp;
  int offset;

  if (p == regdummyptr)
    return;

  // Find last node.
  scan = p;
  for (;;) {
    temp = regnext(scan);
    if (temp == KWSYS_NULLPTR)
      break;
    scan = temp;
  }

  if (OP(scan) == BACK)
    offset = int(scan - val);
  else
    offset = int(val - scan);
  *(scan + 1) = static_cast<char>((offset >> 8) & 0377);
  *(scan + 2) = static_cast<char>(offset & 0377);
}

/*
 - regoptail - regtail on operand of first argument; nop if operandless
 */
void RegExpCompile::regoptail(char* p, const char* val)
{
  // "Operandless" and "op != BRANCH" are synonymous in practice.
  if (p == KWSYS_NULLPTR || p == regdummyptr || OP(p) != BRANCH)
    return;
  regtail(OPERAND(p), val);
}

////////////////////////////////////////////////////////////////////////
//
//  find and friends
//
////////////////////////////////////////////////////////////////////////

/*
 * Utility class for RegularExpression::find().
 */
class RegExpFind
{
public:
  const char* reginput;   // String-input pointer.
  const char* regbol;     // Beginning of input, for ^ check.
  const char** regstartp; // Pointer to startp array.
  const char** regendp;   // Ditto for endp.

  int regtry(const char*, const char**, const char**, const char*);
  int regmatch(const char*);
  int regrepeat(const char*);
};

// find -- Matches the regular expression to the given string.
// Returns true if found, and sets start and end indexes accordingly.
bool RegularExpression::find(char const* string,
                             RegularExpressionMatch& rmatch) const
{
  const char* s;

  rmatch.clear();
  rmatch.searchstring = string;

  if (!this->program) {
    return false;
  }

  // Check validity of program.
  if (UCHARAT(this->program) != MAGIC) {
    // RAISE Error, SYM(RegularExpression), SYM(Internal_Error),
    printf(
      "RegularExpression::find(): Compiled regular expression corrupted.\n");
    return false;
  }

  // If there is a "must appear" string, look for it.
  if (this->regmust != KWSYS_NULLPTR) {
    s = string;
    while ((s = strchr(s, this->regmust[0])) != KWSYS_NULLPTR) {
      if (strncmp(s, this->regmust, this->regmlen) == 0)
        break; // Found it.
      s++;
    }
    if (s == KWSYS_NULLPTR) // Not present.
      return false;
  }

  RegExpFind regFind;

  // Mark beginning of line for ^ .
  regFind.regbol = string;

  // Simplest case:  anchored match need be tried only once.
  if (this->reganch)
    return (
      regFind.regtry(string, rmatch.startp, rmatch.endp, this->program) != 0);

  // Messy cases:  unanchored match.
  s = string;
  if (this->regstart != '\0')
    // We know what char it must start with.
    while ((s = strchr(s, this->regstart)) != KWSYS_NULLPTR) {
      if (regFind.regtry(s, rmatch.startp, rmatch.endp, this->program))
        return true;
      s++;
    }
  else
    // We don't -- general case.
    do {
      if (regFind.regtry(s, rmatch.startp, rmatch.endp, this->program))
        return true;
    } while (*s++ != '\0');

  // Failure.
  return false;
}

/*
 - regtry - try match at specific point
   0 failure, 1 success
 */
int RegExpFind::regtry(const char* string, const char** start,
                       const char** end, const char* prog)
{
  int i;
  const char** sp1;
  const char** ep;

  reginput = string;
  regstartp = start;
  regendp = end;

  sp1 = start;
  ep = end;
  for (i = RegularExpressionMatch::NSUBEXP; i > 0; i--) {
    *sp1++ = KWSYS_NULLPTR;
    *ep++ = KWSYS_NULLPTR;
  }
  if (regmatch(prog + 1)) {
    start[0] = string;
    end[0] = reginput;
    return (1);
  } else
    return (0);
}

/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 * 0 failure, 1 success
 */
int RegExpFind::regmatch(const char* prog)
{
  const char* scan; // Current node.
  const char* next; // Next node.

  scan = prog;

  while (scan != KWSYS_NULLPTR) {

    next = regnext(scan);

    switch (OP(scan)) {
      case BOL:
        if (reginput != regbol)
          return (0);
        break;
      case EOL:
        if (*reginput != '\0')
          return (0);
        break;
      case ANY:
        if (*reginput == '\0')
          return (0);
        reginput++;
        break;
      case EXACTLY: {
        size_t len;
        const char* opnd;

        opnd = OPERAND(scan);
        // Inline the first character, for speed.
        if (*opnd != *reginput)
          return (0);
        len = strlen(opnd);
        if (len > 1 && strncmp(opnd, reginput, len) != 0)
          return (0);
        reginput += len;
      } break;
      case ANYOF:
        if (*reginput == '\0' ||
            strchr(OPERAND(scan), *reginput) == KWSYS_NULLPTR)
          return (0);
        reginput++;
        break;
      case ANYBUT:
        if (*reginput == '\0' ||
            strchr(OPERAND(scan), *reginput) != KWSYS_NULLPTR)
          return (0);
        reginput++;
        break;
      case NOTHING:
        break;
      case BACK:
        break;
      case OPEN + 1:
      case OPEN + 2:
      case OPEN + 3:
      case OPEN + 4:
      case OPEN + 5:
      case OPEN + 6:
      case OPEN + 7:
      case OPEN + 8:
      case OPEN + 9: {
        int no;
        const char* save;

        no = OP(scan) - OPEN;
        save = reginput;

        if (regmatch(next)) {

          //
          // Don't set startp if some later invocation of the
          // same parentheses already has.
          //
          if (regstartp[no] == KWSYS_NULLPTR)
            regstartp[no] = save;
          return (1);
        } else
          return (0);
      }
      //              break;
      case CLOSE + 1:
      case CLOSE + 2:
      case CLOSE + 3:
      case CLOSE + 4:
      case CLOSE + 5:
      case CLOSE + 6:
      case CLOSE + 7:
      case CLOSE + 8:
      case CLOSE + 9: {
        int no;
        const char* save;

        no = OP(scan) - CLOSE;
        save = reginput;

        if (regmatch(next)) {

          //
          // Don't set endp if some later invocation of the
          // same parentheses already has.
          //
          if (regendp[no] == KWSYS_NULLPTR)
            regendp[no] = save;
          return (1);
        } else
          return (0);
      }
      //              break;
      case BRANCH: {

        const char* save;

        if (OP(next) != BRANCH) // No choice.
          next = OPERAND(scan); // Avoid recursion.
        else {
          do {
            save = reginput;
            if (regmatch(OPERAND(scan)))
              return (1);
            reginput = save;
            scan = regnext(scan);
          } while (scan != KWSYS_NULLPTR && OP(scan) == BRANCH);
          return (0);
          // NOTREACHED
        }
      } break;
      case STAR:
      case PLUS: {
        char nextch;
        int no;
        const char* save;
        int min_no;

        //
        // Lookahead to avoid useless match attempts when we know
        // what character comes next.
        //
        nextch = '\0';
        if (OP(next) == EXACTLY)
          nextch = *OPERAND(next);
        min_no = (OP(scan) == STAR) ? 0 : 1;
        save = reginput;
        no = regrepeat(OPERAND(scan));
        while (no >= min_no) {
          // If it could work, try it.
          if (nextch == '\0' || *reginput == nextch)
            if (regmatch(next))
              return (1);
          // Couldn't or didn't -- back up.
          no--;
          reginput = save + no;
        }
        return (0);
      }
      //              break;
      case END:
        return (1); // Success!

      default:
        // RAISE Error, SYM(RegularExpression), SYM(Internal_Error),
        printf(
          "RegularExpression::find(): Internal error -- memory corrupted.\n");
        return 0;
    }
    scan = next;
  }

  //
  //  We get here only if there's trouble -- normally "case END" is the
  //  terminating point.
  //
  // RAISE Error, SYM(RegularExpression), SYM(Internal_Error),
  printf("RegularExpression::find(): Internal error -- corrupted pointers.\n");
  return (0);
}

/*
 - regrepeat - repeatedly match something simple, report how many
 */
int RegExpFind::regrepeat(const char* p)
{
  int count = 0;
  const char* scan;
  const char* opnd;

  scan = reginput;
  opnd = OPERAND(p);
  switch (OP(p)) {
    case ANY:
      count = int(strlen(scan));
      scan += count;
      break;
    case EXACTLY:
      while (*opnd == *scan) {
        count++;
        scan++;
      }
      break;
    case ANYOF:
      while (*scan != '\0' && strchr(opnd, *scan) != KWSYS_NULLPTR) {
        count++;
        scan++;
      }
      break;
    case ANYBUT:
      while (*scan != '\0' && strchr(opnd, *scan) == KWSYS_NULLPTR) {
        count++;
        scan++;
      }
      break;
    default: // Oh dear.  Called inappropriately.
      // RAISE Error, SYM(RegularExpression), SYM(Internal_Error),
      printf("cm RegularExpression::find(): Internal error.\n");
      return 0;
  }
  reginput = scan;
  return (count);
}

/*
 - regnext - dig the "next" pointer out of a node
 */
static const char* regnext(const char* p)
{
  int offset;

  if (p == regdummyptr)
    return (KWSYS_NULLPTR);

  offset = NEXT(p);
  if (offset == 0)
    return (KWSYS_NULLPTR);

  if (OP(p) == BACK)
    return (p - offset);
  else
    return (p + offset);
}

static char* regnext(char* p)
{
  int offset;

  if (p == regdummyptr)
    return (KWSYS_NULLPTR);

  offset = NEXT(p);
  if (offset == 0)
    return (KWSYS_NULLPTR);

  if (OP(p) == BACK)
    return (p - offset);
  else
    return (p + offset);
}

} // namespace KWSYS_NAMESPACE
