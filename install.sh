#!/bin/sh
#set -v

# assure we don't copy to absolute paths / root dir if $INSTROOT is not set:

if [ "${INSTROOT}" != "" ]; then
  INSTROOT=${INSTROOT}/
fi
BINPATH=${INSTROOT}$1
INCPATH=${INSTROOT}$2
MANPATH=${INSTROOT}$3
LIBPATH=${INSTROOT}$4
DOCPATH=${INSTROOT}$5

# this is not a perfect solution, but I don't know a better one at the moment:

if [ -f /usr/X11R6/bin/mkdirhier ] ; then
  MKDIRHIER="/usr/X11R6/bin/mkdirhier"
else
  if [ -f /usr/bin/X11/mkdirhier ] ; then
    MKDIRHIER="/usr/bin/X11/mkdirhier"
  else
    MKDIRHIER="mkdir -p"
  fi
fi

echo "Installing files:"

if [ "${BINPATH}" != "" ]; then
 ${MKDIRHIER} ${BINPATH}
 chmod 755 ${BINPATH}
 for i in asl plist alink pbind p2hex p2bin; do
  #echo copy ${TARG_OBJDIR}$i${TARG_EXEXTENSION} to ${BINPATH}/$i${TARG_EXEXTENSION} ...
  strip ${TARG_OBJDIR}$i${TARG_EXEXTENSION}
  if cp ${TARG_OBJDIR}$i${TARG_EXEXTENSION} ${BINPATH}; then
   chmod 755 ${BINPATH}/$i${TARG_EXEXTENSION}
  fi
 done
fi

if test "${TARG_EXEXTENSION}" = ".exe"; then
  if test "${OBJDIR}" = ""; then
    CPINC=./unumlaut
  else
    CPINC=${OBJDIR}unumlaut
  fi
else
  CPINC=cp
fi

if [ "${INCPATH}" != "" ]; then
 ${MKDIRHIER} ${INCPATH}
 chmod 755 ${INCPATH}
 for path in . avr s12z s12z/vh s12z/vc s12z/vca coldfire st6 st7 stm8 stm8/stm8s stm8/stm8l stm8/stm8af stm8/stm8al stm8/stm8t z8 pdk; do
  if [ "$path" != "." ]; then
   mkdir ${INCPATH}/${path}
   chmod 755 ${INCPATH}/${path}
  fi
  for file in include/${path}/*.inc; do
   base=`basename ${file}`
   #echo copy ${file} to ${INCPATH}/${path}/${base} ...
   if ! ${CPINC} ${file} ${INCPATH}/${path}/ ; then
     exit
   fi
   chmod 644 ${INCPATH}/${path}/$base
  done
 done
fi

if [ "${MANPATH}" != "" ]; then
 ${MKDIRHIER} ${MANPATH}/man1
 chmod 755 ${MANPATH} ${MANPATH}/man1
 for i in man/*.1; do
  #echo copy $i to ${MANPATH}/man1/`basename $i` ...
  if cp $i ${MANPATH}/man1 ; then
   chmod 644 ${MANPATH}/man1/`basename $i`
  fi
 done
fi

if [ "${LIBPATH}" != "" ]; then
 ${MKDIRHIER} ${LIBPATH}
 chmod 755 ${LIBPATH}
 if [ -f as.msg ]; then
  for file in ${TARG_OBJDIR}*.msg; do
   base=`basename ${file}`
   #echo copy ${file} to ${LIBPATH}/${base} ...
   if cp ${file} ${LIBPATH}/ ; then
    chmod 644 ${LIBPATH}/${base}
   fi
  done
 fi
fi

if [ "${DOCPATH}" != "" ]; then
 ${MKDIRHIER} ${DOCPATH}
 chmod 755 ${DOCPATH}
 for i in DE EN; do
  for ext in doc html dvi ps pdf; do
   if [ -f doc_$i/as.${ext} ]; then
    #echo copy doc_$i/as.${ext} to ${DOCPATH}/as-$i.${ext} ...
    cp doc_$i/as.${ext} ${DOCPATH}/as_$i.${ext}
   fi
  done
  chmod 644 ${DOCPATH}/as_$i.*
 done
 cp doc_COM/taborg*.tex ${DOCPATH}
 chmod 644 ${DOCPATH}/taborg*.tex
 cp doc_COM/ps*.tex ${DOCPATH}
 chmod 644 ${DOCPATH}/ps*.tex
 cp doc_COM/biblio.tex ${DOCPATH}
 chmod 644 ${DOCPATH}/biblio.tex
 cp doc_COM/cp3finst.tex ${DOCPATH}
 chmod 644 ${DOCPATH}/cp3finst.tex
 cp COPYING ${DOCPATH}
 chmod 644 ${DOCPATH}/COPYING
fi
