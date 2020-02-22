# exp := val | (baseexp)
# baseexp := let id exp exp
#           | letrec id exp exp
#           | if exp exp exp
#           | aop exp exp 
#           | cmp exp exp
#           | exp {exp}*
# val := \id.exp | @id.exp | atom
# atom := alnum | bool
# alnum := id | num
# bool := true | false
# aop := + | - | * | / | %
# cmp := < | <= | == | != | > | >-

from copy import deepcopy

_Tok_Id = 0
_Tok_Num = 1
_Tok_Bool = 2
_Tok_Aop = 3
_Tok_Cmp = 4
_Tok_Par = 6
_Tok_Keywd = 7
_Tok_Lam = 8
_Tok_Eol = 9
_Tok_Unknown = 10
_Tok_Reclam = 12

def _skipWhitespace(s):
    s = s.lstrip(' \n')
    return s

def _peek(s):
    s = _skipWhitespace(s)
    return s[0]

def _eat(s, c):
    s = _skipWhitespace(s)
    assert s[0] == c, 'c: {0}, s: {1}'.format(c, s)
    s = _skipWhitespace(s[1:])
    return s

def _isTokBool(ts):
    return ts in ['true', 'false']

def _isTokAop(ts):
    return ts in ['+', '-', '*', '/', '%']

def _isTokAsg(ts):
    return ts == ':='

def _isTokCmp(ts):
    return ts in ['<', '<=', '==', '!=', '>', '>=']

def _isTokPar(ts):
    return ts in ['(', ')']

def _isTokLam(ts):
    return ts == '\\'

def _isTokReclam(ts):
    return ts == '@'

def _isTokKeywd(ts):
    return ts in ['if', 'let', 'letrec']

def _nextToken(s):
    tok = _Tok_Unknown
    s = _skipWhitespace(s)
    if len(s):
        ts = _peek(s)
        # single length tokens
        tokEnd = 1
        if _isTokAop(ts):
            # + - * /
            tok = _Tok_Aop

        elif _isTokPar(ts):
            # ( )
            tok = _Tok_Par

        elif _isTokLam(ts):
            # \
            tok = _Tok_Lam

        elif _isTokReclam(ts):
            # @
            tok = _Tok_Reclam

        else:

            ts, tokEnd = '', 0
            singleCharSet = '). \n'
            while (tokEnd < len(s)) and (s[tokEnd] not in singleCharSet):
                tokEnd += 1
            ts = s[:tokEnd]
            if _isTokBool(ts):
                # true false
                tok = _Tok_Bool

            elif _isTokCmp(ts):
                # < == !=
                tok = _Tok_Cmp

            elif _isTokKeywd(ts):
                tok = _Tok_Keywd

            elif ts.isdigit():
                tok = _Tok_Num

            else:
                tok = _Tok_Id

        s = _skipWhitespace(s[tokEnd:])
        return tok, ts, s

    else:
        return _Tok_Eol, '', s

def _visitExpstr(s, v):
    tok, ts, s = _nextToken(s)

    if tok == _Tok_Eol:
        return '', s
    
    elif tok == _Tok_Par:
        val, s = _visitBaseexpstr(s, v)
        s = _eat(s, ')')
        return val, s

    elif tok == _Tok_Lam or tok == _Tok_Reclam:
        createFunc = [v.createLam, v.createReclam][tok == _Tok_Reclam]

        varTok, varName, s = _nextToken(s)
        assert varTok == _Tok_Id

        s = _eat(s, '.')
        # always obtain raw string, does not depend on visitor
        expstr, s = _extractExpstr(s)

        res = createFunc(varName, expstr)
        return res, _skipWhitespace(s)

    else:
        createFunc = None
        if tok == _Tok_Id:
            createFunc = v.createVar
        elif tok == _Tok_Num:
            createFunc = v.createNum
        elif tok == _Tok_Bool:
            createFunc = v.createBool
        else:
            raise Exception("unkown token {0} for value".format(tok))
        
        res = createFunc(ts)
        return res, s

def _visitBaseexpstr(s, v):
    scopy = deepcopy(s)
    tok, ts, s = _nextToken(s)

    if tok == _Tok_Keywd:
        if ts == 'if':

            boolExpres, s = _visitExpstr(s, v)

            # always obtain raw string, does not depend on visitor
            expstr1, s = _extractExpstr(s)
            expstr2, s = _extractExpstr(s)

            res = v.createIfElse(boolExpres, expstr1, expstr2)
            return res, _skipWhitespace(s)

        elif ts == 'let' or ts == 'letrec':
            createFunc = [v.createLet, v.createLetrec][ts == 'letrec']
            
            varTok, varName, s = _nextToken(s)
            assert varTok == _Tok_Id
            # always obtain raw string, does not depend on visitor
            varExpstr, s = _extractExpstr(s)
            expstr, s = _extractExpstr(s)

            # res = v.createLet(varName, varExpstr, expstr)
            res = createFunc(varName, varExpstr, expstr)
            return res, _skipWhitespace(s)

        else:
            raise Exception("unknown keyword: {0}".format(ts))

    elif tok == _Tok_Aop:
        lhsres, s = _visitExpstr(s, v)
        rhsres, s = _visitExpstr(s, v)

        val = v.createAopExp(ts, lhsres, rhsres)
        return val, _skipWhitespace(s)

    elif tok == _Tok_Cmp:
        expres1, s = _visitExpstr(s, v)
        expres2, s = _visitExpstr(s, v)

        res = v.createBexp(ts, expres1, expres2)
        return res, _skipWhitespace(s)
    else:
        s = scopy

        expres, s = _visitExpstr(s, v)
        while len(s) and _peek(s) != ')':
            valstr, s = _extractExpstr(s)

            expres = v.createApp(expres, valstr)

        return expres, _skipWhitespace(s)

def _getAopByTokstr(ts):
    if ts == '+':
        return lambda x, y: x + y
    elif ts == '-':
        return lambda x, y: x - y
    elif ts == '*':
        return lambda x, y: x * y
    elif ts == '/':
        return lambda x, y: x / y
    elif ts == '%':
        return lambda x, y: x % y
    else:
        raise Exception("unknown aop tokstr: {0}".format(ts))

def _getCmpByTokstr(ts):
    if ts == '<':
        return lambda x, y: x < y
    elif ts == '<=':
        return lambda x, y: x <= y
    elif ts == '==':
        return lambda x, y: x == y
    elif ts == '!=':
        return lambda x, y: x != y
    elif ts == '>':
        return lambda x, y: x > y
    elif ts == '>=':
        return lambda x, y: x >= y

    raise Exception("unknown compare tokenstr {0}".format(ts))

class ExtractStrVisitor:
    def name(self):
        return 'extract'
        
    def createBool(self, s):
        return s

    def createBexp(self, cmp, e1, e2):
        return '({0} {1} {2})'.format(cmp, e1, e2)

    def createAopExp(self, aop, e1, e2):
        return '({0} {1} {2})'.format(aop, e1, e2)

    def createApp(self, func, arg):
        return '({0} {1})'.format(func, arg)

    def createNum(self, s):
        return s

    def createVar(self, s):
        return s

    def createLet(self, varName, varExpstr, expstr):
        return '(let {0} {1} {2})'.format(varName, varExpstr, expstr)

    def createLetrec(self, varName, varExpstr, expstr):
        return '(letrec {0} {1} {2})'.format(varName, varExpstr, expstr)

    def createLam(self, varName, expstr):
        return '\\{0}.{1}'.format(varName, expstr)

    def createReclam(self, varName, expstr):
        return '@{0}.{1}'.format(varName, expstr)

    def createIfElse(self, bexp, e1, e2):
        return '(if {0} {1} {2})'.format(bexp, e1, e2)

__extractv__ = ExtractStrVisitor()

def _extractExpstr(s):
    return _visitExpstr(s, __extractv__)

class EvalStrVisitor:
    def __init__(self, env):
        self._env = env

    def name(self):
        return 'evaluate'

    def createBool(self, s):
        return [False, True][s == 'true']

    def createBexp(self, cmpstr, v1, v2):
        cmp = _getCmpByTokstr(cmpstr)
        return cmp(v1, v2)

    def createAopExp(self, aopstr, v1, v2):
        aop = _getAopByTokstr(aopstr)
        return aop(v1, v2)

    def createApp(self, func, arg):
        ((varName, expstr), fenv) = func

        newenv, varenv = deepcopy(fenv), deepcopy(self._env)
        # assert varName not in newenv

        newenv[varName] = (arg, varenv)

        newv = EvalStrVisitor(newenv)
        val, _ = _visitExpstr(expstr, newv)
        return val

    def createNum(self, s):
        return int(s)

    def createVar(self, s):
        (varExpr, varenv) = self._env[s]
        newv = EvalStrVisitor(varenv)
        val, _ = _visitExpstr(varExpr, newv)
        return val

    def createLet(self, varName, varExpstr, expstr):
       varenv, newenv = deepcopy(self._env), deepcopy(self._env)
       # assert varName not in newenv

       newenv[varName] = (varExpstr, varenv)
       newv = EvalStrVisitor(newenv)

       val, _ = _visitExpstr(expstr, newv)
       return val

    def createLetrec(self, varName, varExpstr, expstr):
        recVarExpstr = __extractv__.createReclam(varName, varExpstr)

        newv = deepcopy(self)

        val = newv.createLet(varName, recVarExpstr, expstr)
        return val

    def createLam(self, varName, expstr):
        return ((varName, expstr), deepcopy(self._env))

    def createReclam(self, varName, expstr):
        newenv, varenv = deepcopy(self._env), deepcopy(self._env)
        newenv[varName] = (__extractv__.createReclam(varName, expstr), varenv)

        newv = EvalStrVisitor(newenv)
        val, _ = _visitExpstr(expstr, newv)
        return val

    def createIfElse(self, bval, e1, e2):
        val, _ = _visitExpstr([e2, e1][bval], self)
        return val

def _formatExpr(s, padlen = 0):
    result = ''
    padstr = ''.join([' '] * padlen)

    letkey = ''
    if 'letrec' in s:
        letkey = 'letrec'
    elif 'let' in s:
        letkey = 'let'

    if len(letkey):
        letidx = s.index(letkey) + len(letkey)
        inindex = s.index('in')

        result = '{0}{1}\n'.format(padstr, _skipWhitespace(s[:letidx]))
        result += _formatExpr(_skipWhitespace(s[letidx:inindex]), padlen + 4)
        result += '\n{0}in\n'.format(padstr)
        result += _formatExpr(_skipWhitespace(s[(inindex + 2):]), padlen + 4)
    else:
        result = padstr + s
    return result

def evalExp(s):
    v = EvalStrVisitor({})
    val, _ = _visitExpstr(s, v)
    # return 'expression: \n{0}\nresult: {1}'.format(_formatExpr(s, 4), val)
    return 'expression: \n{0}\nresult: {1}'.format(s, val)

if __name__ == '__main__':
    expstr = '(let f \n\
    \\f.\\x. ( if (== x 0) 1 (* x ( f f (- x 1) ) ) ) \n\
    (let factorial (f f) (factorial 6)) \n\
)'
    print evalExp(expstr)
    print ''

    expstr = '(let Y \n\
    \\f.(\\x.(f (x x)) \\x.(f (x x))) \n\
    (let f \n\
        \\f.\\n.( if (== n 0) 1 (* n ( f (- n 1) )) ) \n\
        (let factorial (Y f) (factorial 6)) \n\
    ) \n\
)'
    print evalExp(expstr)
    print ''

    expstr = '(letrec factorial \n\
    \\x.(if (== x 0) 1 (* x (factorial (- x 1)))) \n\
    (factorial 6) \n\
)'
    print evalExp(expstr)
    print ''

    expstr = '(let SUCC \n\
    \\N.\\f.\\x. (f (N f x)) \n\
    (let f \n\
        \\x.(+ x 1) \n\
        (let TWO \n\
            \\f.\\x.(f (f x)) \n\
            (let FOUR \n\
                \\f.\\x.(f (f (f (f x)))) \n\
                (let ADD \n\
                    \\n1.\\n2.(n1 SUCC n2) \n\
                    (ADD TWO FOUR f 0) \n\
                ) \n\
            ) \n\
        ) \n\
    ) \n\
)'
    print evalExp(expstr)
    print ''

    expstr = '(letrec bin \n\
    \\x.(if (<= x 1) x (+ (* (bin (/ x 2)) 10) (% x 2)) ) \n\
    (bin 4094) \n\
)'
    print evalExp(expstr)
    print ''
    # while True:
    #     expstr = raw_input('ugly lambda$ ')
    #     if expstr == 'exit':
    #         break
    #     elif len(expstr):
    #         print evalExp(expstr)
