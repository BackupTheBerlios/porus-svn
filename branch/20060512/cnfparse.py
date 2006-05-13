
from pyparsing import Word, Literal, Combine, alphas, nums, alphanums, quotedString, ZeroOrMore, Forward, cStyleComment, restOfLine, Group

import logging

debug=logging.debug
warn=logging.warning
error=logging.error

#logging.getLogger().setLevel(logging.DEBUG)

tree={}
treestack=[]
curtree=tree

def debugToks(n,s,l,t):
    debug("%s: loc=%s, tokens=%s"%(n,str(l),str(t)))

def convertHex(s,l,t):
    debugToks("convertHex",s,l,t)
    return [int(t[0],0)]

def convertDec(s,l,t):
    debugToks("convertDec",s,l,t)
    return [int(t[0])]

def convertStr(s,l,t):
    debugToks("convertStr",s,l,t)
    return [t[0][1:-1]]

def convertEnum(s,l,t):
    debugToks("convertEnum",s,l,t)

def addToTree(sym,val):
    global curtree
    if sym in curtree:
	if isinstance(curtree[sym],tuple):
	    curtree[sym]=curtree[sym]+(val,)
	else:
	    curtree[sym]=(curtree[sym],val)
    else:
	curtree[sym]=val

def addAssignment(s,l,t):
    debugToks("addAssignment",s,l,t)
    symbol=t[0][0]
    val=t[0][2]
    addToTree(symbol,val)

def startBlock(s,l,t):
    global curtree,treestack
    debugToks("startBlock",s,l,t)
    symbol=t[0][0]
    d={}
    addToTree(symbol,d)
    treestack.append(curtree)
    curtree=d

def endBlock(s,l,t):
    global curtree,treestack
    debugToks("endBlock",s,l,t)
    curtree=treestack.pop()

symbol=Word(alphas+'_',alphanums+'_')
hexval=Combine(Literal('0x')+Word(nums+'abcdefABCDEF')).setParseAction(convertHex)
decval=Word(nums).setParseAction(convertDec)
enumval=Word(alphas+'_',alphanums+'_').setParseAction(convertEnum)
stringval=quotedString.setParseAction(convertStr)
value=hexval|decval|quotedString|enumval
assignment=Group(symbol+'='+value).setParseAction(addAssignment)
block=Forward()
statement=assignment|block
block<<Group(symbol+'{').setParseAction(startBlock)+ZeroOrMore(statement)+Literal('}').setParseAction(endBlock)
comment=cStyleComment|(Literal('//')+restOfLine)
config=ZeroOrMore(statement).ignore(comment)

def parseFile(s):
    global config, tree, curtree
    tree={}
    curtree=tree
    treestack=[]
    config.parseFile(s)
    return tree

if __name__=='__main__':
    import sys
    parseFile(sys.argv[1])
    print tree
