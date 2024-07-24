md %1
set binfiles=asl.exe plist.exe alink.exe pbind.exe p2hex.exe p2bin.exe
rem for %%i in (%binfiles%) do strip %%i
for %%i in (%binfiles%) do copy %%i %1
set binfiles=
copy *.msg %1 

md %2
for %%i in (include\*.inc) do unumlaut %%i %2\
md %2\avr
for %%i in (include\avr\*.inc) do unumlaut %%i %2\avr\
md %2\ez80
for %%i in (include\ez80\*.inc) do unumlaut %%i %2\ez80\
md %2\s12z
for %%i in (include\s12z\*.inc) do unumlaut %%i %2\s12z\
md %2\s12z\vh
for %%i in (include\s12z\vh\*.inc) do unumlaut %%i %2\s12z\vh\
md %2\s12z\vc
for %%i in (include\s12z\vc\*.inc) do unumlaut %%i %2\s12z\vc\
md %2\s12z\vca
for %%i in (include\s12z\vca\*.inc) do unumlaut %%i %2\s12z\vca\
md %2\coldfire
for %%i in (include\coldfire\*.inc) do unumlaut %%i %2\coldfire\
md %2\st6
for %%i in (include\st6\*.inc) do unumlaut %%i %2\st6\
md %2\st7
for %%i in (include\st7\*.inc) do unumlaut %%i %2\st7\
md %2\stm8
for %%i in (include\stm8\*.inc) do unumlaut %%i %2\stm8\
md %2\stm8\stm8s
for %%i in (include\stm8\stm8s\*.inc) do unumlaut %%i %2\stm8\stm8s\
md %2\stm8\stm8l
for %%i in (include\stm8\stm8l\*.inc) do unumlaut %%i %2\stm8\stm8l\
md %2\stm8\stm8af
for %%i in (include\stm8\stm8af\*.inc) do unumlaut %%i %2\stm8\stm8af\
md %2\stm8\stm8al
for %%i in (include\stm8\stm8al\*.inc) do unumlaut %%i %2\stm8\stm8al\
md %2\stm8\stm8t
for %%i in (include\stm8\stm8t\*.inc) do unumlaut %%i %2\stm8\stm8t\
md %2\z8
for %%i in (include\z8\*.inc) do unumlaut %%i %2\z8\
md %2\pdk
for %%i in (include\pdk\*.inc) do unumlaut %%i %2\pdk\

md %3
for %%i in (man\*.1) do copy %%i %3

md %4
for %%i in (*.msg) do copy %%i %1

md %5
set docdirs=DE EN
for %%i in (%docdirs%) do copy doc_%%i\as.doc %5\as_%%i.doc
for %%i in (%docdirs%) do copy doc_%%i\as.tex %5\as_%%i.tex
for %%i in (%docdirs%) do copy doc_%%i\as.html %5\as_%%i.html
copy doc_COM\taborg*.tex %5
copy doc_COM\ps*.tex %5
copy COPYING %5
