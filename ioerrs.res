;* ioerrs.res
;*****************************************************************************
;* AS-Portierung                                                             *
;*                                                                           *
;* Definition Fehlermeldungs-Strings                                         *
;*                                                                           *
;* Historie:  6. 9.1996 Grundsteinlegung                                     *
;*           19. 9.1996 ...EACCESS                                           *
;*                                                                           *
;*****************************************************************************

Include header.res

Message IoErr_EPERM
 "kein Eigent&auml;mer"
 "Not owner"
 
Message IoErr_ENOENT
 "Datei oder Verzeichnis nicht gefunden"
 "No such file or directory"

Message IoErr_ESRCH
 "Proze&szlig; existiert nicht"
 "No such process"

Message IoErr_EINTR
 "unterbrochener Systemaufruf"
 "Interrupted system call"

Message IoErr_EIO
 "E/A-Fehler"
 "I/O error"
 
Message IoErr_ENXIO
 "Ger&auml;t existiert nicht"
 "No such device or address"

Message IoErr_E2BIG
 "Parameterliste zu lang"
 "Exec format error"

Message IoErr_ENOEXEC
 "Programm-Dateiformatfehler"
 "Bad file number"

Message IoErr_EBADF
 "ung&uuml;ltiger Dateihandle"
 "No children"

Message IoErr_ECHILD
 "Tochterproze&szlig; existiert nicht"
 "Operation would cause deadlock"

Message IoErr_EDEADLK
 "Operation w&uuml;rde Deadlock verursachen"
 "Operation would cause deadlock"

Message IoErr_ENOMEM
 "Speicher&uuml;berlauf"
 "Not enough core"

Message IoErr_EACCES
 "Zugriff verweigert"
 "Permission denied"

Message IoErr_EFAULT
 "Adre&szlig;fehler"
 "Bad address"

Message IoErr_ENOTBLK
 "Blockger&auml;t erforderlich"
 "Block device required"

Message IoErr_EBUSY
 "Ger&auml;t blockiert"
 "Device or resource busy"

Message IoErr_EEXIST
 "Datei existiert bereits"
 "File exists"

Message IoErr_EXDEV
 "Link &uuml;ber verschiedene Ger&auml;te"
 "Cross-device link"

Message IoErr_ENODEV
 "Ger&auml;t nicht vorhanden"
 "No such device"

Message IoErr_ENOTDIR
 "kein Verzeichnis"
 "Not a directory"

Message IoErr_EISDIR
 "Verzeichnis"
 "Is a directory"

Message IoErr_EINVAL
 "ung&uuml;ltiges Argument"
 "Invalid argument"

Message IoErr_ENFILE
 "Dateitabellen&uuml;berlauf"
 "File table overflow"

Message IoErr_EMFILE
 "zu viele offene Dateien"
 "Too many open files"

Message IoErr_ENOTTY
 "kein Fernschreiber"
 "Not a typewriter"

Message IoErr_ETXTBSY
 "Code-Datei blockiert"
 "Text file busy"

Message IoErr_EFBIG
 "Datei zu gro&szlig;"
 "File too large" 

Message IoErr_ENOSPC
 "Dateisystem voll"
 "No space left on device"

Message IoErr_ESPIPE
 "Ung&uuml;ltige Positionierung"
 "Illegal seek"

Message IoErr_EROFS
 "Dateisystem nur lesbar"
 "Read-only file system"

Message IoErr_EMLINK
 "zu viele Links"
 "Too many links"

Message IoErr_EPIPE
 "geplatzter Schlauch ;-)"
 "Broken pipe"

Message IoErr_EDOM
 "Funktionsargument au&szlig;erhalb Definitionsbereich"
 "Math argument out of domain of func"

Message IoErr_ERANGE
 "Funktionsergebnis au&szlig;erhalb Wertebereich"
 "Math result not representable"

Message IoErr_ENAMETOOLONG
 "Dateiname zu lang"
 "File name too long"

Message IoErr_ENOLCK
 "Datensatzverieglung nicht m&ouml;glich"
 "No record locks available"

Message IoErr_ENOSYS
 "Funktion nicht implementiert"
 "Function not implemented"

Message IoErr_ENOTEMPTY
 "Verzeichnis nicht leer"
 "Directory not empty"

Message IoErr_ELOOP
 "zu viele symbolische Links"
 "Too many symbolic links encountered"

Message IoErr_EWOULDBLOCK
 "Operation w&uuml;rde blockieren"
 "Operation would block"

Message IoErr_ENOMSG
 "keine Nachricht gew&uuml;nschten Typs verf&uuml;gbar"
 "No message of desired type"

Message IoErr_EIDRM
 "Kennung entfernt"
 "Identifier removed"

Message IoErr_ECHRNG
 "Kanalnummer au&szlig;erhalb Bereich"
 "Channel number out of range"

Message IoErr_EL2NSYNC
 "Ebene 2 nicht synchronisiert"
 "Level 2 not synchronized"

Message IoErr_EL3HLT
 "Ebene 3 angehalten"
 "Level 3 halted"

Message IoErr_EL3RST
 "Ebene 3 zur&uuml;ckgesetzt"
 "Level 3 reset"

Message IoErr_ELNRNG
 "Link-Nummer au&szlig;erhalb Bereich"
 "Link number out of range"

Message IoErr_EUNATCH
 "Protokolltreiber nicht angebunden"
 "Protocol driver not attached"

Message IoErr_ENOCSI
 "keine CSI-Struktur verf&uuml;gbar"
 "No CSI structure available"

Message IoErr_READERROR
 "Lesefehler"
 "Read error"

Message IoErrUnknown
 "unbekannter Fehler Nr."
 "unknown error no."
