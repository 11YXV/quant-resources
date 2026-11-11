# writes doc_dwr.txt / doc_var.txt / doc_ceb.txt (the header comment of each generated study)
COMMON = open(__file__.replace('mkdocs.py', 'doc_common.txt')).read()
HEAD = {}
HEAD['dwr'] = open(__file__.replace('mkdocs.py', 'head_dwr.txt')).read()
HEAD['var'] = open(__file__.replace('mkdocs.py', 'head_var.txt')).read()
HEAD['ceb'] = open(__file__.replace('mkdocs.py', 'head_ceb.txt')).read()
for k, v in HEAD.items():
    open(__file__.replace('mkdocs.py', f'doc_{k}.txt'), 'w').write(v.rstrip('\n') + '\n\n' + COMMON.replace('@TAGNAME@', k.upper()))
