# $Id: extractlinks.sed,v 1.2 1994-09-22 11:53:34 srk Exp $
/^MESSAGERETURN/{
s/MESSAGERETURN */"/
s/(.*/",/
h
s/"//g
i\
#ifdef MESSAGE2
G
p
i\
#endif
g
s/"//g
s/,/(va_list);/
i\
#ifdef MESSAGE1
i\
MESSAGERETURN 
p
i\
#endif
d
}
d
